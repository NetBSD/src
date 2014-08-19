#	$NetBSD: Makefile,v 1.2.12.1 2014/08/19 23:52:21 tls Exp $
.include <bsd.own.mk>

# We need more modular flag e.g. we can add dtrace to osnet later

.if ( ${MKZFS} != "no" || ${MKDTRACE} != "no" || ${MKCTF} != "no")
SUBDIR+= osnet
.endif

.include <bsd.subdir.mk>

