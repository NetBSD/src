#	$NetBSD: Makefile.inc,v 1.6.4.1 2004/05/11 14:52:00 tron Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# gmon sources
.PATH: ${.CURDIR}/gmon ${ARCHDIR}/gmon

.-include "${ARCHDIR}/gmon/Makefile.inc"

SRCS+=	gmon.c mcount.c
MAN+=	moncontrol.3
MLINKS+=moncontrol.3 monstartup.3

# mcount and gmon cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po

gmon.po: gmon.o
	cp gmon.o gmon.po
