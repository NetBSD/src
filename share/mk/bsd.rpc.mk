#	$NetBSD: bsd.rpc.mk,v 1.2 2003/05/07 13:41:22 yamt Exp $

# Resolve rpcgen's path, to allow it to be a dependency.

RPCGEN?=	rpcgen
RPC_XDIR?=	${.CURDIR}/

_RPCGEN:=	${RPCGEN:M*rpcgen}
.if ${_RPCGEN:M/*} == ""
_RPCGEN!=	type ${RPCGEN} | awk '{print $$NF}'
.endif


# We don't use implicit suffix rules here to avoid dependencies in the
# Installed files.

.if defined(RPC_INCS)

.for I in ${RPC_INCS}
${I}: ${I:.h=.x} ${_RPCGEN}
	${RPCGEN} -C -h ${RPC_XDIR}${I:.h=.x} -o ${.TARGET}
.endfor

CLEANFILES += ${RPC_INCS}

.depend: ${RPC_INCS}

.endif

.if defined(RPC_XDRFILES)

.for I in ${RPC_XDRFILES}
${I}: ${RPC_XDIR}${I:_xdr.c=.x} ${_RPCGEN}
	${RPCGEN} -C -c ${RPC_XDIR}${I:_xdr.c=.x} -o ${.TARGET}
.endfor

CLEANFILES += ${RPC_XDRFILES}

.depend: ${RPC_XDRFILES}

.endif

.if defined(RPC_SVCFILES)

.for I in ${RPC_SVCCLASS}
_RPCS += -s ${I}
.endfor

.for I in ${RPC_SVCFILES}

${I}: ${RPC_XDIR}${I:_svc.c=.x} ${_RPCGEN}
	${RPCGEN} -C ${_RPCS} ${RPC_SVCFLAGS} ${RPC_XDIR}${I:_svc.c=.x} \
		-o ${.TARGET}
.endfor

CLEANFILES += ${RPC_SVCFILES}

.depend: ${RPC_SVCFILES}

.endif
