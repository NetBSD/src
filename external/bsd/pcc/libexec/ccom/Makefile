#	$NetBSD: Makefile,v 1.3 2009/09/04 00:50:05 gmcgarry Exp $

.include <bsd.own.mk>

# USE_FORT?= yes

DIST=	${NETBSDSRCDIR}/external/bsd/pcc/dist/pcc
SUBDIST=${DIST}/cc/ccom

PROG=	ccom
SRCS=   cgram.y
SRCS+=  optim.c pftn.c scan.c trees.c inline.c symtabs.c
SRCS+=	gcc_compat.c init.c local.c code.c stabs.c match.c reader.c optim2.c
SRCS+=	regs.c local2.c order.c table.c common.c main.c external.c

MAN=    ccom.1

BINDIR= /usr/libexec

CPPFLAGS+=	-I${SUBDIST} -I${.CURDIR} -I${DIST}/mip
CPPFLAGS+=	-I${.CURDIR}/../..
CPPFLAGS+=	-I${.OBJDIR}
CPPFLAGS+=	-DPCC_DEBUG -DGCC_COMPAT
YFLAGS+=	-d

COPTS.scan.c += -Wno-error

DPSRCS=	external.h

CLEANFILES+=	external.h external.c ccom.cat1 cgram.h mkext scan.c
CLEANFILES+=    ccom.cat1


.include "../../Makefile.inc"

trees.c: pass2.h

pass2.h: external.h

optim2.o reader.o: external.h

.include <bsd.prog.mk>

.PATH:	${SUBDIST} ${MIPDIR} ${MDIR}

external.h external.c: ${MIPDIR}/mkext.c $(MDIR)/table.c
	$(HOST_CC) $(DEFS) $(CPPFLAGS) $(CFLAGS) -DMKEXT -o ${.OBJDIR}/mkext ${MIPDIR}/mkext.c \
	$(MDIR)/table.c ${MIPDIR}/common.c
	${.OBJDIR}/mkext
