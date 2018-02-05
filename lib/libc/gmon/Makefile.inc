#	$NetBSD: Makefile.inc,v 1.13 2018/02/05 22:12:37 mrg Exp $
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


# mcount and gmon cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po

gmon.po: gmon.o
	cp gmon.o gmon.po
