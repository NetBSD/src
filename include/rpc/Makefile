#	$NetBSD: Makefile,v 1.7 2001/10/31 20:58:28 tv Exp $
#

INCS=	auth.h auth_unix.h clnt.h clnt_soc.h nettype.h \
	pmap_clnt.h pmap_prot.h pmap_rmt.h raw.h rpc.h \
	rpc_msg.h rpcb_clnt.h rpcb_prot.h rpcent.h \
	svc.h svc_auth.h svc_soc.h types.h xdr.h

CLEANFILES+=	rpcb_prot.h

INCSDIR=	/usr/include/rpc

.include <bsd.prog.mk>

rpcb_prot.h: rpcb_prot.x ${RPCGEN}
	${RPCGEN} -C -h ${.CURDIR}/rpcb_prot.x -o ${.TARGET}
