#	$NetBSD: Makefile,v 1.6 2024/09/24 11:13:41 rin Exp $
#
LIB	= libtos.a

OBJS	= biosrw.o ahdi.o aout.o bsdlabel.o bsdstart.o cread.o diskio.o elf.o \
	  eprintf.o error.o sysinfo.o xalloc.o xexit.o keywait.o

HEADERS	= ahdilbl.h disklbl.h diskio.h kparamb.h libtos.h

include ../Makefile.inc

${LIB}: ${OBJS}
	@${RM} ${LIB}
	${AR} ${LIB} ${OBJS}

.if ${HAVE_GCC:U0} >= 12
# ADDR_* defined in tosdefs.h are in the 0-th page, even if 4KB page,
# i.e., [0, 0x1000). This causes -Warray-bounds for GCC12 and later.
COPTS.sysinfo.c+=	-Wno-error=array-bounds
.endif
