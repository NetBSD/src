#	$Id: Makefile,v 1.1 1999/03/26 22:25:40 scottr Exp $
#
# Makefile for loadable iwm device driver kernel module
#

.include "../Makefile.inc"

.PATH:		$S/arch/mac68k/obio

CFILES=		iwm_mod.c iwm_fd.c
SFILES=		iwm.s

SRCS=		${CFILES} ${SFILES}

KMOD=		iwmfd

MAN=

CLEANFILES+=	*~ ${KMOD}

#WARNS=		1
#CPPFLAGS=	-DDIAGNOSTIC -DDDB -DDEBUG -nostdinc -Dmc68020 -Dmac68k
CPPFLAGS=	-DDIAGNOSTIC -DDDB -nostdinc -Dmc68020 -Dmac68k
CFLAGS= 	-g -O -m68020-40

AFLAGS=		-x assembler-with-cpp -traditional-cpp


.include <bsd.kmod.mk> 


