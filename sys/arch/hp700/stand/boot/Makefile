#	$NetBSD: Makefile,v 1.4.56.1 2008/02/18 21:04:33 mjf Exp $

#	$OpenBSD: Makefile,v 1.10 2000/05/31 19:56:51 mickey Exp $

PROG=		boot
PROGSOURCE=	srt0.S boot.c bootinfo.c conf.c tgets.c
NEWVERSWHAT=	"LIF/FFS/LFS Boot"

CLEANFILES+=	boot.gdb boot.map

# IODC can handle only 2GB, and need not to use 64bit integer
CPPFLAGS+=	-D__daddr_t=int32_t

#CPPFLAGS+=	-DEXEC_DEBUG -DPDCDEBUG
#CPPFLAGS+=	-DLIFDEBUG -DDEBUGBUG -DDEBUG
CPPFLAGS+=	-DBOOTARG_APIVER=3
CPPFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes
WARNS=	2

.include "../Makefile.buildboot"
