#	$NetBSD: bsd.rpc.mk,v 1.4 2003/07/10 10:34:37 lukem Exp $

RPC_XDIR?=	${.CURDIR}/

# We don't use implicit suffix rules here to avoid dependencies in the
# Installed files.

.if defined(RPC_INCS)

.for I in ${RPC_INCS}
${I}: ${I:.h=.x}
	${TOOL_RPCGEN} -C -h ${RPC_XDIR}${I:.h=.x} -o ${.TARGET}
.endfor

CLEANFILES += ${RPC_INCS}

.depend: ${RPC_INCS}

.endif

.if defined(RPC_XDRFILES)

.for I in ${RPC_XDRFILES}
${I}: ${RPC_XDIR}${I:_xdr.c=.x}
	${TOOL_RPCGEN} -C -c ${RPC_XDIR}${I:_xdr.c=.x} -o ${.TARGET}
.endfor

CLEANFILES += ${RPC_XDRFILES}

.depend: ${RPC_XDRFILES}

.endif

.if defined(RPC_SVCFILES)

.for I in ${RPC_SVCCLASS}
_RPCS += -s ${I}
.endfor

.for I in ${RPC_SVCFILES}

${I}: ${RPC_XDIR}${I:_svc.c=.x}
	${TOOL_RPCGEN} -C ${_RPCS} ${RPC_SVCFLAGS} ${RPC_XDIR}${I:_svc.c=.x} \
		-o ${.TARGET}
.endfor

CLEANFILES += ${RPC_SVCFILES}

.depend: ${RPC_SVCFILES}

.endif
