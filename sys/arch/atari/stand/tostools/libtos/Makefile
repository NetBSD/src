#	$NetBSD: Makefile,v 1.1.1.1.44.3 2002/06/23 17:35:21 jdolecek Exp $
#
LIB	= libtos.a

OBJS	= biosrw.o ahdi.o aout.o bsdlabel.o bsdstart.o cread.o diskio.o elf.o \
	  eprintf.o error.o sysinfo.o xalloc.o xexit.o keywait.o

HEADERS	= ahdilbl.h disklbl.h diskio.h kparamb.h libtos.h

include ../Makefile.inc

${LIB}: ${OBJS}
	@${RM} ${LIB}
	${AR} ${LIB} ${OBJS}
