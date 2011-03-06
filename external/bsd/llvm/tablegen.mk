#	$NetBSD: tablegen.mk,v 1.1.2.2 2011/03/06 15:07:39 bouyer Exp $

.include <bsd.own.mk>

.for t in ${TABLEGEN_SRC}
.for f in ${TABLEGEN_OUTPUT} ${TABLEGEN_OUTPUT.${t}}
${f:C,\|.*$,,}: ${t}
	[ -z "${f:C,\|.*$,,}" ] || mkdir -p ${f:C,\|.*$,,:H}
	${TOOL_TBLGEN} -I${LLVM_SRCDIR}/include ${TABLEGEN_INCLUDES} \
	    ${TABLEGEN_INCLUDES.${t}} ${f:C,^.*\|,,:C,\^, ,} \
	    ${.ALLSRC:M*/${t}} > ${.TARGET}.tmp && mv ${.TARGET}.tmp ${.TARGET}
DPSRCS+=	${f:C,\|.*$,,}
CLEANFILES+=	${f:C,\|.*$,,}
.endfor
.endfor
