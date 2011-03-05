#	$NetBSD: Makefile.inc,v 1.1.4.2 2011/03/05 20:53:05 rmind Exp $

GENASSYM_CONF=	${S}/dev/arcbios/genassym.cf

.if !target(assym.h)
assym.h: ${GENASSYM_CONF} ${GENASSYM_EXTRA}
	${_MKTARGET_CREATE}
	cat ${GENASSYM_CONF} ${GENASSYM_EXTRA} | \
	    ${TOOL_GENASSYM} -- ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    > assym.h.tmp && \
	mv -f assym.h.tmp assym.h
${SRCS:C/\.[Ss]/.o/} ${SRCS:C/\.[Ss]/.d/}: assym.h
.endif
