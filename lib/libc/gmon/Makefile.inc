#	$NetBSD: Makefile.inc,v 1.5.2.1 2002/01/28 20:50:39 nathanw Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# gmon sources
.PATH: ${.CURDIR}/gmon ${ARCHDIR}/gmon

.-include "${ARCHDIR}/gmon/Makefile.inc"

SRCS+=	gmon.c mcount.c
MAN+=	moncontrol.3
MLINKS+=moncontrol.3 monstartup.3

# mcount cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po
