#	$NetBSD: Makefile,v 1.1 2014/07/26 19:30:44 dholland Exp $
#
# sysinst is usually built when the crunched install binary is built,
# but can be built here as a normal program for testing.

SUBDIR = arch/${MACHINE}

.if ALL_MACHINES

SUBDIR  = arch/acorn26
SUBDIR += arch/acorn32
SUBDIR += arch/alpha
SUBDIR += arch/amd64
SUBDIR += arch/amiga
SUBDIR += arch/arc
SUBDIR += arch/atari
SUBDIR += arch/bebox
SUBDIR += arch/cats
SUBDIR += arch/cobalt
SUBDIR += arch/emips
SUBDIR += arch/evbarm
SUBDIR += arch/evbmips
SUBDIR += arch/evbppc
SUBDIR += arch/ews4800mips
SUBDIR += arch/hp300
SUBDIR += arch/hpcarm
SUBDIR += arch/hpcmips
SUBDIR += arch/hpcsh
SUBDIR += arch/hppa
SUBDIR += arch/i386
SUBDIR += arch/landisk
SUBDIR += arch/mac68k
SUBDIR += arch/macppc
SUBDIR += arch/mipsco
SUBDIR += arch/mvme68k
SUBDIR += arch/news68k
SUBDIR += arch/newsmips
SUBDIR += arch/ofppc
SUBDIR += arch/playstation2
SUBDIR += arch/pmax
SUBDIR += arch/prep
SUBDIR += arch/sandpoint
SUBDIR += arch/sgimips
SUBDIR += arch/shark
SUBDIR += arch/sparc
SUBDIR += arch/sparc64
SUBDIR += arch/vax
SUBDIR += arch/x68k
SUBDIR += arch/zaurus

# XXX notyet SUBDIR += arch/iyonix
.endif

install:

.include <bsd.subdir.mk>
