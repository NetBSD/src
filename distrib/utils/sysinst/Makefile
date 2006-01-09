#	$NetBSD: Makefile,v 1.8 2006/01/09 10:03:51 tsutsui Exp $
#
# sysinst is usually built when the crunched install binary is built,
# but can be built here as a normal program for testing.

SUBDIR = arch/${MACHINE}

.if ALL_MACHINES
SUBDIR = arch/acorn26 arch/acorn32 arch/alpha arch/amd64
SUBDIR += arch/amiga arch/arc arch/atari arch/bebox
SUBDIR += arch/cats arch/evbarm arch/evbppc arch/ews4800mips arch/hp300
SUBDIR += arch/hpcmips arch/i386 arch/mac68k arch/macppc
SUBDIR += arch/mipsco arch/mvme68k arch/news68k arch/newsmips
SUBDIR += arch/pc532 arch/playstation2 arch/pmax arch/prep
SUBDIR += arch/sandpoint arch/sgimips arch/shark arch/sparc
SUBDIR += arch/sparc64 arch/vax arch/x68k
# XXX notyet SUBDIR += arch/iyonix
.endif


.include <bsd.subdir.mk>
