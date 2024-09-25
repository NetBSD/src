#	$NetBSD: Makefile,v 1.7 2024/09/25 08:32:44 rin Exp $
#
LIB	= libtos.a

OBJS	= biosrw.o ahdi.o aout.o bsdlabel.o bsdstart.o cread.o diskio.o elf.o \
	  eprintf.o error.o sysinfo.o xalloc.o xexit.o keywait.o

HEADERS	= ahdilbl.h disklbl.h diskio.h kparamb.h libtos.h

include ../Makefile.inc

${LIB}: ${OBJS}
	@${RM} ${LIB}
	${AR} ${LIB} ${OBJS}
