#	$NetBSD: Makefile,v 1.6 2008/10/25 22:27:38 apb Exp $
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

AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -I$S/arch

iwm.o: iwm.s
	${CPP} ${AFLAGS} ${CPPFLAGS} ${.IMPSRC} | ${TOOL_SED} -e 's/^#.*//' | \
		${AS} -o ${.TARGET}

.include <bsd.kmod.mk>
