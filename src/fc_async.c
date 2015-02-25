#include <fc_core.h>
#include <fc_async.h>
extern struct settings settings;

void async_init()
{
	int i=0;
	nfree_preadbufq = 0;

	uint8_t * bufp = fc_mmap( settings.slab_size * PREADBUF_COUNT );
		ASSERT(bufp != NULL);
	memset(bufp,0xff,settings.slab_size * PREADBUF_COUNT);

	struct preadbuf* preadbufp =fc_alloc(sizeof(*preadbufp) * PREADBUF_COUNT );
		ASSERT(preadbufp != NULL);

	TAILQ_INIT(&free_preadbufq);
	for(i=0 ; i < PREADBUF_COUNT ; i++)
	{
		preadbufp[i].bufp = bufp + (settings.slab_size * i);
		TAILQ_INSERT_HEAD(&free_preadbufq, &preadbufp[i], entry);
		nfree_preadbufq++;
	}
//For debug purpose
	mcount= 0;
	dcount= 0;
}

void async_conn_init(struct context* ctx, struct conn* c)
{
	struct epoll_event event;
	int status;

	c->pread_t = 0;
	c->pread_iop =&(c->pread_io);
	c->pread_efd = eventfd(0, EFD_NONBLOCK);

	event.events = (uint32_t)(EPOLLIN | EPOLLET);
	event.data.ptr = c;
	status = epoll_ctl(ctx->ep,EPOLL_CTL_ADD,c->pread_efd, &event);
	if (status < 0) {
	                log_error("epoll ctl on e %d pread_efd %d failed: %s", ctx->ep, c->pread_efd,
	                          strerror(errno));
	        }

	memset(&(c->pread_ctx), 0 , sizeof(c->pread_ctx));
	io_queue_init(1, &(c->pread_ctx));
}

void async_conn_close(struct context *ctx, struct conn *conn)
{
	close(conn->pread_efd);
    io_queue_release(conn->pread_ctx);
    epoll_ctl(ctx->ep, EPOLL_CTL_DEL, conn->pread_efd, NULL);
}

void async_pread_callback(io_context_t ctx, struct iocb *iocb, long res, long res2)
{
  if (res2 != 0) {
	  	  	fprintf(stderr, "async pread error\n");
	  	  	exit(1);
  	  }
  if (res != iocb->u.c.nbytes) {
            fprintf(stderr, "expect : %lu\n read : %ld\n", iocb->u.c.nbytes, res);
            exit(1);
      }
}

//I had to make these metadata static to global, because I have to use it in async_pread()
extern struct slabclass *ctable;       /* table of slabclass indexed by cid */
extern struct slabinfo *stable;        	/* table of slabinfo indexed by sid */
extern int ssd_fd;
extern off_t slab_to_daddr(struct slabinfo *sinfo);
//extern uint32_t nstable;               /* # slab table entry */

void async_pread( struct context* ctx, struct conn* conn,struct itemx* itx, struct msg* msg)
{
 struct slabclass *c;    /* slab class */
 //struct item *it;        /* item */
 struct slabinfo *sinfo; /* slab info */
 struct epoll_event event;
 //int n;                  /* bytes read */
 off_t off;              /* offset to read from */
 //size_t size;            /* size to read */
 off_t aligned_off;      /* aligned offset to read from */
 size_t aligned_size;    /* aligned size to read */
 uint32_t	sid = itx->sid;
 uint32_t	addr = itx->offset;

 ASSERT(sid < nstable);
 ASSERT(addr < settings.slab_size);

 sinfo = &stable[sid];
 c = &ctable[sinfo->cid];
 //size = settings.slab_size;
 //it = NULL;

  off = slab_to_daddr(sinfo) + addr;
  aligned_off = ROUND_DOWN(off, 512);
  aligned_size = ROUND_UP((c->size + (off - aligned_off)), 512);

  //Allocate preadbuffer for each connection , I'll return this resource in async_pread_post()
  ASSERT( !TAILQ_EMPTY(&free_preadbufq) );
  	if(nfree_preadbufq==0)
		{exit(-1);}
  nfree_preadbufq--;
  conn->pread_buf= TAILQ_FIRST(&free_preadbufq);
  TAILQ_REMOVE(&free_preadbufq,conn->pread_buf,entry);

  //Set one iocb structure with pread information, callback fucntion, eventfd
  io_prep_pread(&(conn->pread_io), ssd_fd, conn->pread_buf->bufp, aligned_size, aligned_off);
  io_set_callback(&(conn->pread_io), async_pread_callback);
  io_set_eventfd(&(conn->pread_io), conn->pread_efd);

  //Register event to epoll descriptor
  event.events = (uint32_t)(EPOLLIN | EPOLLET);
  event.data.ptr = conn;
  epoll_ctl(ctx->ep, EPOLL_CTL_MOD, conn->pread_efd, &event);

  //Actual trigger async pread
  io_submit(conn->pread_ctx, 1, &(conn->pread_iop) );

  //For async_pread_post(), I saved meta_data.
  conn->pread_t = 1;
  conn->pread_cas = itx->cas;
  conn->pread_off = off;
  conn->pread_aligned_off = aligned_off;
  conn->pread_msg = msg;
}

void async_pread_post(struct context *ctx, struct conn *conn)
{
	struct item* it;
	unsigned long int value;

	conn->pread_t = 0;
	io_queue_run(conn->pread_ctx); 		   //Call async_pread_callback() to verify pread is done right.
	eventfd_read(conn->pread_efd, &value);     //Verify event is actually triggered, and I want to re-use the event_efd .
	ASSERT(value == 1);
	it = (struct item *)(conn->pread_buf->bufp + (conn->pread_off - conn->pread_aligned_off));
	    	//debug code
	    	//printf("it->magic: %0x\n",it->magic);
	    	//printf("preadbuf: %p\n", conn->pread_buf->bufp);
	    	//printf("pread_off: %d\n", conn->pread_off);
	    	//printf("pread_aligned_off: %d\n", conn->pread_aligned_off);
	ASSERT(it->magic == ITEM_MAGIC);

	rsp_send_value(ctx, conn, conn->pread_msg, it, conn->pread_cas ); //Return preadbuf to free_preadbufq.
	nfree_preadbufq++;
	TAILQ_INSERT_HEAD(&free_preadbufq,conn->pread_buf,entry);
}
