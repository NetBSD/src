#	$NetBSD: Makefile,v 1.8 2003/01/05 19:24:09 sommerfeld Exp $

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

.include <bsd.prog.mk>
.include <bsd.rpc.mk>
