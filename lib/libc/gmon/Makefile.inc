#	$NetBSD: Makefile.inc,v 1.15 2021/05/30 02:24:05 joerg Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# gmon sources
.PATH: ${.CURDIR}/gmon ${ARCHDIR}/gmon

.-include "${ARCHDIR}/gmon/Makefile.inc"

SRCS+=	gmon.c mcount.c
MAN+=	moncontrol.3
MLINKS+=moncontrol.3 monstartup.3

.if (${MACHINE_CPU} == "mips")
# Turn off as(1) warnings on MIPS, since warnings are fatal with WARNS>0
# and mcount.c causes warnings from as(1).
# mcount.c should be fixed and this test removed.
#
COPTS.mcount.c+=${${ACTIVE_CXX} == "gcc":? -Wa,--no-warn :}
.endif

.if (${MACHINE_CPU} == "i386" || ${MACHINE_CPU} == "x86_64") && \
    ${HAVE_GCC:U0} >= 6
# The usage of __builtin_frame_address(1) should be OK.
COPTS.mcount.c+=${${ACTIVE_CXX} == "gcc":? -Wno-error=frame-address :}
.endif
COPTS.mcount.c+=${${ACTIVE_CC} == "clang":? -Wno-error=frame-address :}


# mcount and gmon cannot be compiled with profiling
GMONOBJEXT=${${MKPIC} != "no":?pico:o}
mcount.po: mcount.${GMONOBJEXT}
	cp mcount.${GMONOBJEXT} mcount.po

gmon.po: gmon.${GMONOBJEXT}
	cp gmon.${GMONOBJEXT} gmon.po
