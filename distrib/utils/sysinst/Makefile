#	$NetBSD: Makefile,v 1.4 2003/06/03 11:54:48 dsl Exp $
#
# can't make here due to crunchgen
#

SUBDIR = arch/${MACHINE}

.if ALL_MACHINES
SUBDIR = arch/acorn26 arch/acorn32 arch/alpha arch/amd64 arch/arc
SUBDIR += arch/bebox arch/cats arch/evbarm arch/evbppc arch/hp300
SUBDIR += arch/hpcmips arch/i386 arch/macppc arch/mipsco
SUBDIR += arch/mvme68k arch/news68k arch/newsmips arch/pc532
SUBDIR += arch/playstation2 arch/pmax
SUBDIR += arch/sandpoint arch/sgimips arch/shark
SUBDIR += arch/sparc arch/sparc64 arch/vax
.endif

.if SYSINSTLANG
.if ${SYSINSTLANG} != fr && ${SYSINSTLANG} != pl
SUBDIR += arch/atari
.endif
.if ${SYSINSTLANG} != fr
SUBDIR += arch/mac68k arch/prep arch/x68k
.endif
.if ${SYSINSTLANG} != pl
SUBDIR += arch/amiga
.endif
.endif

.include <bsd.subdir.mk>
