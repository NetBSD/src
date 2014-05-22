#	$NetBSD: Makefile.inc,v 1.1.10.1 2014/05/22 11:40:19 yamt Exp $

GENASSYM_CONF=	${S}/dev/arcbios/genassym.cf

.if !target(assym.h)
assym.h: ${GENASSYM_CONF} ${GENASSYM_EXTRA}
	${_MKTARGET_CREATE}
	cat ${GENASSYM_CONF} ${GENASSYM_EXTRA} | \
	    ${TOOL_GENASSYM} -- ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    ${GENASSYM_CPPFLAGS} > assym.h.tmp && \
	mv -f assym.h.tmp assym.h
${SRCS:C/\.[Ss]/.o/} ${SRCS:C/\.[Ss]/.d/}: assym.h
.endif
