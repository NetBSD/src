#	$NetBSD: Makefile,v 1.2 2001/10/10 14:19:49 leo Exp $
#
LIB	= libtos.a

OBJS	= aout.o bsdstart.o elf.o eprintf.o error.o sysinfo.o xalloc.o \
	  xexit.o keywait.o

HEADERS	= kparamb.h libtos.h

include ../Makefile.inc

${LIB}: ${OBJS}
	@${RM} ${LIB}
	${AR} ${LIB} ${OBJS}
