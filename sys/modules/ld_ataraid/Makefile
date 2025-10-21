#	$NetBSD: Makefile,v 1.2 2025/10/21 04:35:29 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ata

KMOD=	ld_ataraid
IOCONF=	ld_ataraid.ioconf

SRCS+=	ld_ataraid.c

CPPFLAGS+=	-DNBIO	# Enable bioctl(8)

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
