#	$NetBSD: bsd.eabi.mk,v 1.1 2012/08/03 08:02:47 matt Exp $

MLIBDIR=	eabi

COPTS+=		-mabi=aapcs-linux
CPUFLAGS+=	-mabi=aapcs-linux
LDADD+=		-mabi=aapcs-linux
LDFLAGS+=	-mabi=aapcs-linux
MKDEPFLAGS+=	-mabi=aapcs-linux

.include "${.PARSEDIR}/../../Makefile.compat"
