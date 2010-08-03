#	$NetBSD: Makefile.inc,v 1.2 2010/08/03 17:24:45 drochner Exp $
#

# libc rpc using rump syscalls
SRCS+=	bindresvport.c clnt_bcast.c clnt_dg.c clnt_generic.c clnt_vc.c \
	rpc_generic.c rpc_soc.c rpcb_clnt.c \
	svc.c svc_dg.c svc_generic.c svc_run.c svc_vc.c

CPPFLAGS+=	-DPORTMAP -D_REENTRANT -DRUMP_SYS_NETWORKING
CPPFLAGS+=	-DDEBUG -DLIBWRAP
