#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:06 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ata

KMOD=	ld_ataraid
IOCONF=	ld_ataraid.ioconf

SRCS+=	ld_ataraid.c

CPPFLAGS+=	-DNBIO	# Enable bioctl(8)

.include <bsd.kmodule.mk>
