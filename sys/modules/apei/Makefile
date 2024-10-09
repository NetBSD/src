#	$NetBSD: Makefile,v 1.1.4.2 2024/10/09 13:00:12 martin Exp $
#

.include "../Makefile.inc"

.PATH:	$S/dev/acpi

KMOD=	apei
IOCONF=	apei.ioconf

SRCS+=	apei.c
SRCS+=	apei_bert.c
SRCS+=	apei_einj.c
SRCS+=	apei_erst.c
SRCS+=	apei_hest.c
SRCS+=	apei_interp.c
SRCS+=	apei_mapreg.c
SRCS+=	apei_reg.c

.include <bsd.kmodule.mk>
