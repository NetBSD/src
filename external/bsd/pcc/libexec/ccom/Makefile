#	$NetBSD: Makefile,v 1.7 2011/04/14 09:31:25 plunky Exp $

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

MKEXTDIR!=	cd ../mkext && ${PRINTOBJDIR}

external.c external.h:	${MKEXTDIR}/mkext
	${_MKTARGET_CREATE}
	${MKEXTDIR}/mkext

CLEANFILES+=	external.c external.h

.include <bsd.prog.mk>
