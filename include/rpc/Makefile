#	$NetBSD: Makefile,v 1.4 2001/01/07 06:56:15 enami Exp $
#

INCS=	auth.h auth_unix.h clnt.h clnt_soc.h nettype.h \
	pmap_clnt.h pmap_prot.h pmap_rmt.h raw.h rpc.h \
	rpc_msg.h rpcb_clnt.h rpcb_prot.h rpcent.h \
	svc.h svc_auth.h svc_soc.h types.h xdr.h

.ifnmake getrpcgen
XRPCGEN != (cd ${.CURDIR} && ${MAKE} -B ${MAKEFLAGS} getrpcgen)
.endif

getrpcgen:
	@set -- X `type ${RPCGEN}` && shift $$(($$# - 1)) && echo "$$1"

rpcb_prot.h:: rpcb_prot.x ${XRPCGEN}
	${RPCGEN} -h ${.CURDIR}/rpcb_prot.x -o ${.TARGET}

CLEANFILES+=	rpcb_prot.h

INCSDIR=	/usr/include/rpc

.include <bsd.prog.mk>
