#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/4/93
#	$Id: Makefile.inc,v 1.3 1994/12/22 10:15:35 cgd Exp $

# gmon sources
.PATH: ${.CURDIR}/gmon

SRCS+=	gmon.c mcount.c
MAN+=	moncontrol.3

# mcount cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po
