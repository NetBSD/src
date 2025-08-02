#	$NetBSD: Makefile,v 1.5.238.1 2025/08/02 05:55:31 perseant Exp $
#
LIB	= libtos.a

OBJS	= biosrw.o ahdi.o aout.o bsdlabel.o bsdstart.o cread.o diskio.o elf.o \
	  eprintf.o error.o sysinfo.o xalloc.o xexit.o keywait.o

HEADERS	= ahdilbl.h disklbl.h diskio.h kparamb.h libtos.h

include ../Makefile.inc

${LIB}: ${OBJS}
	@${RM} ${LIB}
	${AR} ${LIB} ${OBJS}
