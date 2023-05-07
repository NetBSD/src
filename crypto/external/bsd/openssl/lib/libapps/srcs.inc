#	$NetBSD: srcs.inc,v 1.1 2023/05/07 16:22:10 christos Exp $

APPSINCS= \
apps_lib.inc \

.for appsinc in ${APPSINCS}
.if exists(${.CURDIR}/arch/${APPS_MACHINE_CPU}/${appsinc})
.include "${.CURDIR}/arch/${APPS_MACHINE_CPU}/${appsinc}"
.else
.include "${appsinc}"
.endif
.endfor
