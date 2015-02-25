###############################################################################
#                                                                             #
#                     README file for async fatache                           #
#                              v0.1                                           #
#                                                                             #
###############################################################################



fatcache
--------
Fatcache is a SSD version of memcached.
I replaced sync_pread to async_pread when handling get operation.
You can find more details about fatcache in README_original.md

Source
------
Source code of fatcache is written in C.
You can find most of my changes in fc_async.c and fc_async.h

Binary
------
Executable binary is /src/fatcache

Compile & Build
---------------
If you don't want a debug configuration, just type this command.
	#sh build.sh

Execution 
---------------
You need to modify options for fatcache in src/test.sh and type this command.
	#sh src/server.sh

Benchmark
---------------
After fatcache server is up, you can benchmark fatcache using memaslap, to do that need to install libmemcached first.
	#yum install libmemcached

You need to modify options for memaslap in src/memaslap.sh and type this command.
	#sh src/memaslap.sh

