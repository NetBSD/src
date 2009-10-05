#	$NetBSD: Makefile,v 1.1 2009/10/05 22:44:26 haad Exp $
.include <bsd.own.mk>

# We need more modular flag e.g. we can add dtrace to osnet later

.if ( ${MKZFS} != "no")
SUBDIR+= osnet
.endif

.include <bsd.subdir.mk>

