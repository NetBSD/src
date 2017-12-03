#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:56 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ata

KMOD=	ld_ataraid
IOCONF=	ld_ataraid.ioconf

SRCS+=	ld_ataraid.c

CPPFLAGS+=	-DNBIO	# Enable bioctl(8)

.include <bsd.kmodule.mk>
