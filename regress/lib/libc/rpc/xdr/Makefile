#	$NetBSD: Makefile,v 1.10 2003/08/01 17:04:00 lukem Exp $

NOMAN=		# defined

PROG=		xdrtest
RPCSRCS=	testbits.x
SRCS=		${RPCSRCS:.x=_xdr.c} xdrtest.c
DPSRCS=		${RPCSRCS:.x=.h}
CLEANFILES+=	${RPCSRCS:.x=.h} ${RPCSRCS:.x=_xdr.c}
CPPFLAGS+=	-I.
RPC_INCS=	${RPCSRCS:.x=.h}
RPC_XDRFILES=	${RPCSRCS:.x=_xdr.c}

regress: ${PROG}
	./${PROG}

.include <bsd.rpc.mk>
.include <bsd.prog.mk>
