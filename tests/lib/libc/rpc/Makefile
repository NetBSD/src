# $NetBSD: Makefile,v 1.4 2023/06/03 09:09:15 lukem Exp $

MKMAN=	no

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/lib/libc/rpc

TESTS_C=		t_xdr
SRCS.t_xdr=		${RPCSRCS:.x=_xdr.c} t_xdr.c

TESTS_C+=		t_rpc

RPCSRCS=		h_testbits.x
DPSRCS=			${RPCSRCS:.x=.h}
CLEANFILES+=	${RPCSRCS:.x=.h} ${RPCSRCS:.x=_xdr.c}
CPPFLAGS+=		-I.
RPC_INCS=		${RPCSRCS:.x=.h}
RPC_XDRFILES=	${RPCSRCS:.x=_xdr.c}

COPTS.t_rpc.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.rpc.mk>
.include <bsd.test.mk>

