#	$NetBSD: Makefile,v 1.3 2001/10/11 07:07:42 leo Exp $
#
LIB	= libtos.a

OBJS	= aout.o bsdstart.o cread.o elf.o eprintf.o error.o sysinfo.o \
	  xalloc.o xexit.o keywait.o

HEADERS	= kparamb.h libtos.h

include ../Makefile.inc

${LIB}: ${OBJS}
	@${RM} ${LIB}
	${AR} ${LIB} ${OBJS}
