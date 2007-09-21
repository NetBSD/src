#	$NetBSD: Makefile,v 1.1 2007/09/21 21:10:55 abs Exp $

.include <bsd.own.mk>

# USE_FORT?= yes

DIST=	${NETBSDSRCDIR}/dist/pcc
SUBDIST=${DIST}/cc/ccom

PROG=	ccom
SRCS=   cgram.y
SRCS+=  optim.c pftn.c scan.c trees.c inline.c symtabs.c
SRCS+=	gcc_compat.c init.c local.c code.c stabs.c match.c reader.c optim2.c
SRCS+=	regs.c local2.c order.c table.c common.c main.c external.c

BINDIR= /usr/libexec

CPPFLAGS+=	-I${SUBDIST} -I${.CURDIR} -I${DIST}/mip
CPPFLAGS+=	-DPCC_DEBUG -DGCC_COMPAT
YFLAGS+=	-d

CLEANFILES+=	external.h external.c ccom.cat1 cgram.h mkext

trees.c: pass2.h

pass2.h: external.h

optim2.o reader.o: external.h

.include <bsd.prog.mk>

.PATH:	${SUBDIST} ${MIPDIR} ${MDIR}

external.h external.c: ${MIPDIR}/mkext.c $(MDIR)/table.c
	$(CC) $(DEFS) $(CPPFLAGS) $(CFLAGS) -DMKEXT -o mkext ${MIPDIR}/mkext.c \
	$(MDIR)/table.c ${MIPDIR}/common.c
	./mkext
