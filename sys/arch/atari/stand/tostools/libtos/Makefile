#	$NetBSD: Makefile,v 1.3.4.2 2002/02/28 04:08:31 nathanw Exp $
#
LIB	= libtos.a

OBJS	= biosrw.o ahdi.o aout.o bsdstart.o cread.o diskio.o elf.o eprintf.o \
	  error.o sysinfo.o xalloc.o xexit.o keywait.o

HEADERS	= ahdilbl.h diskio.h kparamb.h libtos.h

include ../Makefile.inc

${LIB}: ${OBJS}
	@${RM} ${LIB}
	${AR} ${LIB} ${OBJS}
