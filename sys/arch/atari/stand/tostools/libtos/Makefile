#	$NetBSD: Makefile,v 1.1.1.1.44.1 2002/01/10 19:40:21 thorpej Exp $
#
LIB	= libtos.a

OBJS	= aout.o bsdstart.o cread.o elf.o eprintf.o error.o sysinfo.o \
	  xalloc.o xexit.o keywait.o

HEADERS	= kparamb.h libtos.h

include ../Makefile.inc

${LIB}: ${OBJS}
	@${RM} ${LIB}
	${AR} ${LIB} ${OBJS}
