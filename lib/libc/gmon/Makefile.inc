#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/4/93
#	$Id: Makefile.inc,v 1.2 1994/05/14 06:31:10 cgd Exp $

# gmon sources
.PATH: ${.CURDIR}/gmon

SRCS+=	gmon.c mcount.c
MAN3+=	moncontrol.0

# mcount cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po
