#	$NetBSD: tablegen.mk,v 1.3 2011/09/12 13:32:59 joerg Exp $

.include <bsd.own.mk>

.for t in ${TABLEGEN_SRC}
.for f in ${TABLEGEN_OUTPUT} ${TABLEGEN_OUTPUT.${t}}
${f:C,\|.*$,,}: ${t} ${TOOL_TBLGEN}
	[ -z "${f:C,\|.*$,,}" ] || mkdir -p ${f:C,\|.*$,,:H}
	${TOOL_TBLGEN} -I${LLVM_SRCDIR}/include ${TABLEGEN_INCLUDES} \
	    ${TABLEGEN_INCLUDES.${t}} ${f:C,^.*\|,,:C,\^, ,} \
	    ${.ALLSRC:M*/${t}} -d ${.TARGET}.d -o ${.TARGET}
DPSRCS+=	${f:C,\|.*$,,}
CLEANFILES+=	${f:C,\|.*$,,} ${f:C,\|.*$,,:C,$,.d,}

.sinclude "${f:C,\|.*$,,:C,$,.d,}"
.endfor
.endfor
