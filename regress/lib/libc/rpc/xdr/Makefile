#	$NetBSD: Makefile,v 1.4 2002/02/10 13:22:58 bjh21 Exp $

PROG=		xdrtest
RPCSRCS=	testbits.x
SRCS=		${RPCSRCS:.x=.c} ${RPCSRCS:.x=.h} xdrtest.c
NOMAN=		# defined
CPPFLAGS+=	-I.

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>

# Resolve rpcgen's path, to allow it to be a dependency.
_RPCGEN:=	${RPCGEN:M*rpcgen}
.if ${_RPCGEN:M/*} == ""
_RPCGEN!=	type ${RPCGEN} | awk '{print $$NF}'
.endif

.SUFFIXES: .x

.x.c: ${_RPCGEN}
	${RPCGEN} -C -c ${.IMPSRC} -o ${.TARGET}
.x.h: ${_RPCGEN}
	${RPCGEN} -C -h ${.IMPSRC} -o ${.TARGET}
