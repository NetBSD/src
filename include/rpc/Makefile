#	$NetBSD: Makefile,v 1.3 2000/09/10 23:21:49 erh Exp $
#

INCS=	auth.h auth_unix.h clnt.h clnt_soc.h nettype.h \
	pmap_clnt.h pmap_prot.h pmap_rmt.h raw.h rpc.h \
	rpc_msg.h rpcb_clnt.h rpcb_prot.h rpcent.h \
	svc.h svc_auth.h svc_soc.h types.h xdr.h

.ifnmake getrpcgen
XRPCGEN != (cd ${.CURDIR} && ${MAKE} getrpcgen)
.endif

getrpcgen:
	@set -- X `type ${RPCGEN}` && shift $$(($$# - 1)) && echo "$$1"

rpcb_prot.h:: rpcb_prot.x ${XRPCGEN}
	${RPCGEN} -h ${.CURDIR}/rpcb_prot.x -o ${.TARGET}

CLEANFILES+=	rpcb_prot.h

INCSDIR=	/usr/include/rpc

.include <bsd.prog.mk>
