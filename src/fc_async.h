#ifndef _FC_ASYNC_H_
#define _FC_ASYNC_H_

#include <fc_core.h>

//Jason start preadbuf
#define PREADBUF_COUNT 1024

struct preadbuf{
	TAILQ_ENTRY(preadbuf) entry;
	uint8_t* bufp;
};
TAILQ_HEAD(preadbufh,preadbuf);
struct preadbufh free_preadbufq;
int nfree_preadbufq;
//Jason end preadbuf


void async_init();
void async_conn_init(struct context* ctx, struct conn* c);
void async_conn_close(struct context *ctx, struct conn *conn);
void async_pread( struct context* ctx, struct conn* conn,struct itemx* itx, struct msg* msg);
void async_pread_callback(io_context_t ctx, struct iocb *iocb, long res, long res2);
void async_pread_post(struct context *ctx, struct conn *conn);


#endif
