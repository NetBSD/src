#	$NetBSD: Makefile,v 1.9 2003/07/29 02:45:23 lukem Exp $

NOMAN=		# defined

PROG=		xdrtest
RPCSRCS=	testbits.x
SRCS=		${RPCSRCS:.x=_xdr.c} xdrtest.c
DPSRCS=		${RPCSRCS:.x=.h}
CLEANFILES+=	${RPCSRCS:.x=_xdr.c}
CPPFLAGS+=	-I.
RPC_INCS=	${RPCSRCS:.x=.h}
RPC_XDRFILES=	${RPCSRCS:.x=_xdr.c}

regress: ${PROG}
	./${PROG}

.include <bsd.rpc.mk>
.include <bsd.prog.mk>
