#	$NetBSD: Makefile,v 1.4.2.1 2013/06/23 06:28:34 tls Exp $

.include <bsd.own.mk>

# crtstuff is built out of elsewhere, or not at all
SUBDIR+= libgcc libiberty libobjc libgomp
SUBDIR+= libmudflap libmudflapth

.if (${MKLIBSTDCXX} != "no")
SUBDIR+= libsupc++ libstdc++-v3
.endif

.include <bsd.subdir.mk>
