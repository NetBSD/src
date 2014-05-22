#	$NetBSD: Makefile,v 1.2.6.1 2014/05/22 14:01:22 yamt Exp $
.include <bsd.own.mk>

# We need more modular flag e.g. we can add dtrace to osnet later

.if ( ${MKZFS} != "no" || ${MKDTRACE} != "no" || ${MKCTF} != "no")
SUBDIR+= osnet
.endif

.include <bsd.subdir.mk>

