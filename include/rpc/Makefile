#	$NetBSD: Makefile,v 1.5.2.3 2002/02/23 23:01:40 gmcgarry Exp $
#

INCS=	auth.h auth_unix.h clnt.h clnt_soc.h nettype.h \
	pmap_clnt.h pmap_prot.h pmap_rmt.h raw.h rpc.h \
	rpc_msg.h rpcb_clnt.h rpcb_prot.h rpcent.h \
	svc.h svc_auth.h svc_soc.h types.h xdr.h

CLEANFILES+=	rpcb_prot.h

INCSDIR=	/usr/include/rpc

.include <bsd.prog.mk>

# Resolve rpcgen's path, to allow it to be a dependency.
_RPCGEN:=	${RPCGEN:M*rpcgen}
.if ${_RPCGEN:M/*} == ""
_RPCGEN!=	type ${RPCGEN} | awk '{print $$NF}'
.endif

rpcb_prot.h: rpcb_prot.x ${_RPCGEN}
	${RPCGEN} -C -h ${.CURDIR}/rpcb_prot.x -o ${.TARGET}
