#	$NetBSD: srcs.inc,v 1.1.2.2 2023/08/11 13:41:54 martin Exp $

APPSINCS= \
apps_lib.inc \

.for appsinc in ${APPSINCS}
.if exists(${.CURDIR}/arch/${APPS_MACHINE_CPU}/${appsinc})
.include "${.CURDIR}/arch/${APPS_MACHINE_CPU}/${appsinc}"
.else
.include "${appsinc}"
.endif
.endfor
