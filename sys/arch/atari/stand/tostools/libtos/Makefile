#	$NetBSD: Makefile,v 1.1.1.1.44.2 2002/03/16 15:56:56 jdolecek Exp $
#
LIB	= libtos.a

OBJS	= biosrw.o ahdi.o aout.o bsdstart.o cread.o diskio.o elf.o eprintf.o \
	  error.o sysinfo.o xalloc.o xexit.o keywait.o

HEADERS	= ahdilbl.h diskio.h kparamb.h libtos.h

include ../Makefile.inc

${LIB}: ${OBJS}
	@${RM} ${LIB}
	${AR} ${LIB} ${OBJS}
