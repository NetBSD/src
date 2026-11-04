#	$NetBSD: Makefile,v 1.1.58.1 2026/11/04 23:27:55 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ata

KMOD=	ld_ataraid
IOCONF=	ld_ataraid.ioconf

SRCS+=	ld_ataraid.c

CPPFLAGS+=	-DNBIO	# Enable bioctl(8)

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
