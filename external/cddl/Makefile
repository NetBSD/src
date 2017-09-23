#	$NetBSD: Makefile,v 1.3.18.1 2017/09/23 17:50:43 snj Exp $
.include <bsd.own.mk>

# We need more modular flag e.g. we can add dtrace to osnet later

.if (${MKDTRACE} != "no")
SUBDIR+= dtracetoolkit
.endif

.if ( ${MKZFS} != "no" || ${MKDTRACE} != "no" || ${MKCTF} != "no")
SUBDIR+= osnet
.endif

.include <bsd.subdir.mk>

