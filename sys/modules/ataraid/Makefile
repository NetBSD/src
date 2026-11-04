#	$NetBSD: Makefile,v 1.1.58.1 2026/11/04 23:27:53 perseant Exp $

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
