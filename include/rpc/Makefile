#	$NetBSD: Makefile,v 1.1 2000/06/03 11:23:55 fvdl Exp $
#

INCS=	auth.h auth_unix.h clnt.h clnt_soc.h nettype.h \
	pmap_clnt.h pmap_prot.h pmap_rmt.h raw.h rpc.h \
	rpc_msg.h rpcb_clnt.h rpcb_prot.h rpcent.h \
	svc.h svc_auth.h svc_soc.h types.h xdr.h


rpcb_prot.h: rpcb_prot.x
	rpcgen -h ${.ALLSRC} -o ${.TARGET}

CLEANFILES+=	rpcb_prot.h

INCSDIR=	/usr/include/rpc

.include <bsd.prog.mk>
