#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:08 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ata

KMOD=	ld_ataraid
IOCONF=	ld_ataraid.ioconf

SRCS+=	ld_ataraid.c

CPPFLAGS+=	-DNBIO	# Enable bioctl(8)

.include <bsd.kmodule.mk>
