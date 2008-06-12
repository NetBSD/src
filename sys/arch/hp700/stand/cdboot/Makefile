#	$NetBSD: Makefile,v 1.2 2008/06/12 14:04:53 skrll Exp $

PROG=		cdboot
PROGSOURCE=	srt0.S cdboot.c conf.c
NEWVERSWHAT=	"CD Boot"

CLEANFILES+=	boot.gdb boot.map

# IODC can handle only 2GB, and need not to use 64bit integer
CPPFLAGS+=	-D__daddr_t=int32_t

#CPPFLAGS+=	-DDEBUGBUG
#CPPFLAGS+=	-DPDCDEBUG
#CPPFLAGS+=	-DEXEC_DEBUG
CPPFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes
WARNS=	2

.include "../Makefile.buildboot"
