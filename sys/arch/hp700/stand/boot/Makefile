#	$NetBSD: Makefile,v 1.6.10.1 2008/05/16 02:22:23 yamt Exp $

#	$OpenBSD: Makefile,v 1.10 2000/05/31 19:56:51 mickey Exp $

PROG=		boot
PROGSOURCE=	srt0.S boot.c bootinfo.c ct.c conf.c lf.c lif.c tgets.c
NEWVERSWHAT=	"LIF/FFS/LFS Boot"

CLEANFILES+=	boot.gdb boot.map

# IODC can handle only 2GB, and need not to use 64bit integer
CPPFLAGS+=	-D__daddr_t=int32_t

#CPPFLAGS+=	-DEXEC_DEBUG -DPDCDEBUG
#CPPFLAGS+=	-DLIFDEBUG -DDEBUGBUG -DDEBUG
CPPFLAGS+=	-DBOOTARG_APIVER=3
CPPFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes
WARNS=	2

CPPFLAGS+=		-D__INTERNAL_LIBSA_CREAD -DCOMPAT_UFS
SAMISCMAKEFLAGS+="SA_USE_CREAD=yes"

.include "../Makefile.buildboot"
