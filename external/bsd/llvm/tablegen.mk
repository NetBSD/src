#	$NetBSD: tablegen.mk,v 1.1.4.1 2011/06/23 14:18:30 cherry Exp $

.include <bsd.own.mk>

.for t in ${TABLEGEN_SRC}
.for f in ${TABLEGEN_OUTPUT} ${TABLEGEN_OUTPUT.${t}}
${f:C,\|.*$,,}: ${t} ${TOOL_TBLGEN}
	[ -z "${f:C,\|.*$,,}" ] || mkdir -p ${f:C,\|.*$,,:H}
	${TOOL_TBLGEN} -I${LLVM_SRCDIR}/include ${TABLEGEN_INCLUDES} \
	    ${TABLEGEN_INCLUDES.${t}} ${f:C,^.*\|,,:C,\^, ,} \
	    ${.ALLSRC:M*/${t}} -d ${.TARGET}.d.tmp -o ${.TARGET}.tmp \
	    && mv ${.TARGET}.tmp ${.TARGET} && \
	    mv ${.TARGET}.d.tmp ${.TARGET}.d
DPSRCS+=	${f:C,\|.*$,,}
CLEANFILES+=	${f:C,\|.*$,,} ${f:C,\|.*$,,:C,$,.d,}

.sinclude "${f:C,\|.*$,,:C,$,.d,}"
.endfor
.endfor
