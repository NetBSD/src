#	$NetBSD: bsd.eabi.mk,v 1.1.4.2 2012/10/30 18:46:17 yamt Exp $

MLIBDIR=	eabi

COPTS+=		-mabi=aapcs-linux
CPUFLAGS+=	-mabi=aapcs-linux
LDADD+=		-mabi=aapcs-linux
LDFLAGS+=	-mabi=aapcs-linux
MKDEPFLAGS+=	-mabi=aapcs-linux

.include "${.PARSEDIR}/../../Makefile.compat"
