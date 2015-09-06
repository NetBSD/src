# $NetBSD: ldscript.mk,v 1.1 2015/09/06 06:41:14 uebayasi Exp $

# Give MD generated ldscript dependency on ${SYSTEM_OBJ}
.if defined(KERNLDSCRIPT)
.if target(${KERNLDSCRIPT})
${KERNLDSCRIPT}: ${SYSTEM_OBJ}
.endif
.endif

.if defined(KERNLDSCRIPT)
.for k in ${KERNELS}
EXTRA_CLEAN+=	${k}.ldscript
${k}: ${k}.ldscript
${k}.ldscript: ${KERNLDSCRIPT} assym.h
	${_MKTARGET_CREATE}
	${CPP} -I. ${KERNLDSCRIPT} | grep -v '^#' | grep -v '^$$' >$@
.endfor
LINKSCRIPT=	-T ${.TARGET}.ldscript
.endif
