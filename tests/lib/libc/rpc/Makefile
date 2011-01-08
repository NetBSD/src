# $NetBSD: Makefile,v 1.1 2011/01/08 06:59:37 pgoyette Exp $

MKMAN=	no

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/lib/libc/rpc

TESTS_C=		t_xdr
SRCS.t_xdr=		${RPCSRCS:.x=_xdr.c} t_xdr.c

RPCSRCS=		h_testbits.x
DPSRCS=			${RPCSRCS:.x=.h}
CLEANFILES+=	${RPCSRCS:.x=.h} ${RPCSRCS:.x=_xdr.c}
CPPFLAGS+=		-I.
RPC_INCS=		${RPCSRCS:.x=.h}
RPC_XDRFILES=	${RPCSRCS:.x=_xdr.c}

.include <bsd.rpc.mk>
.include <bsd.test.mk>

