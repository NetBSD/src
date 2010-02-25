#	$NetBSD: Makefile,v 1.2 2010/02/25 00:14:10 darran Exp $
.include <bsd.own.mk>

# We need more modular flag e.g. we can add dtrace to osnet later

.if ( ${MKZFS} != "no" || ${MKDTRACE} != "no" )
SUBDIR+= osnet
.endif

.include <bsd.subdir.mk>

