#	$NetBSD: Makefile,v 1.6 2002/03/09 01:35:46 pk Exp $

PROG=		xdrtest
RPCSRCS=	testbits.x
SRCS=		${RPCSRCS:.x=.c} xdrtest.c
DPSRCS=		${RPCSRCS:.x=.h}
CLEANFILES+=	${RPCSRCS:.x=.c}
CPPFLAGS+=	-I.
NOMAN=		# defined

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>

# Resolve rpcgen's path, to allow it to be a dependency.
_RPCGEN:=	${RPCGEN:M*rpcgen}
.if ${_RPCGEN:M/*} == ""
_RPCGEN!=	type ${RPCGEN} | awk '{print $$NF}'
.endif

# Explicit dependency on generated header file
testbits.o xdrtest.o: testbits.h

.SUFFIXES: .x

.x.c: ${_RPCGEN}
	${RPCGEN} -C -c ${.IMPSRC} -o ${.TARGET}
.x.h: ${_RPCGEN}
	${RPCGEN} -C -h ${.IMPSRC} -o ${.TARGET}
