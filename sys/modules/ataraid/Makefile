#	$NetBSD: Makefile,v 1.2 2025/10/21 04:35:29 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ata

KMOD=	ataraid
IOCONF=	ataraid.ioconf

SRCS+=	ata_raid.c
SRCS+=	ata_raid_subr.c
SRCS+=	ata_raid_promise.c
SRCS+=	ata_raid_adaptec.c
SRCS+=	ata_raid_nvidia.c
SRCS+=	ata_raid_via.c
SRCS+=	ata_raid_jmicron.c
SRCS+=	ata_raid_intel.c

# -Wsign-compare issues
WARNS=  3

.include <bsd.kmodule.mk>
