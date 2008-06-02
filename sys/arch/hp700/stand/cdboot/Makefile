#	$NetBSD: Makefile,v 1.1.6.2 2008/06/02 13:22:07 mjf Exp $

PROG=		cdboot
PROGSOURCE=	srt0.S cdboot.c conf.c
NEWVERSWHAT=	"CD Boot"

.PATH:	${.CURDIR}/../boot

CLEANFILES+=	boot.gdb boot.map

# IODC can handle only 2GB, and need not to use 64bit integer
CPPFLAGS+=	-D__daddr_t=int32_t

#CPPFLAGS+=	-DDEBUGBUG
#CPPFLAGS+=	-DPDCDEBUG
#CPPFLAGS+=	-DEXEC_DEBUG
CPPFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes
WARNS=	2

.include "../Makefile.buildboot"
