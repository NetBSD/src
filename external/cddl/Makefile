#	$NetBSD: Makefile,v 1.3 2014/03/25 09:52:55 ozaki-r Exp $
.include <bsd.own.mk>

# We need more modular flag e.g. we can add dtrace to osnet later

.if ( ${MKZFS} != "no" || ${MKDTRACE} != "no" || ${MKCTF} != "no")
SUBDIR+= osnet
.endif

.include <bsd.subdir.mk>

