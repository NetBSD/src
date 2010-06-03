#	$NetBSD: Makefile,v 1.6 2010/06/03 19:17:22 plunky Exp $

WARNS?=	2

.include <bsd.init.mk>

.PATH:	${PCC_DIST}/cc/ccom \
	${PCC_DIST}/arch/${TARGMACH} \
	${PCC_DIST}/mip

PROG=	ccom

SRCS=   cgram.y
SRCS+=	scan.l
SRCS+=	optim.c pftn.c trees.c inline.c symtabs.c
SRCS+=	gcc_compat.c init.c local.c code.c stabs.c builtins.c
SRCS+=	match.c reader.c optim2.c regs.c local2.c order.c table.c
SRCS+=	common.c main.c external.c

MAN=    ccom.1

# generate cgram.h
YHEADER=

CPPFLAGS+=	-DPCC_DEBUG
CPPFLAGS+=	-I${.OBJDIR}
CPPFLAGS+=	-I${PCC_DIST}/mip
CPPFLAGS+=	-I${PCC_DIST}/os/${TARGOS}
CPPFLAGS+=	-I${PCC_DIST}/arch/${TARGMACH}
CPPFLAGS+=	-I${PCC_DIST}/cc/ccom

DPSRCS=	external.c external.h

external.c external.h: mkext.c table.c common.c
	${HOST_CC} ${CFLAGS} ${CPPFLAGS} -DMKEXT -o ${.OBJDIR}/mkext ${.ALLSRC}
	${.OBJDIR}/mkext

CLEANFILES+=	external.c external.h mkext

.include <bsd.prog.mk>
