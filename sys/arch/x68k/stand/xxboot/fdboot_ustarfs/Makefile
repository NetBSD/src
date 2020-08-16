#	$NetBSD: Makefile,v 1.1 2020/08/16 06:43:44 isaki Exp $

FS=		ustarfs
BOOTCPPFLAGS+=	-DXXBOOT_$(FS:tu)

# ustarfs requires printf...
BOOTSRCS+=	printf.c subr_prf.c panic.c closeall.c consio1.c

TOUCHPROG=	@true

.include "../Makefile.xxboot"
