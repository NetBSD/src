#	$NetBSD: Makefile,v 1.2 1994/10/17 00:11:18 cgd Exp $

M?=	${MACHINE}

TARG=	INSTALL.$M
SRC=	${.CURDIR}/INSTALL
DEP=	${SRC} ${.CURDIR}/mirrors ${.CURDIR}/$M/whatis ${.CURDIR}/$M/contents \
	${.CURDIR}/$M/hardware ${.CURDIR}/$M/xfer ${.CURDIR}/$M/prep \
	${.CURDIR}/$M/install ${.CURDIR}/$M/upgrade ${.CURDIR}/donations \
	${.CURDIR}/legal.common ${.CURDIR}/$M/legal

${TARG}: ${DEP}
	cpp -I${.CURDIR}/$M -DMACHINE=$M -U$M -C -P ${SRC} > $@

clean cleandir:
	/bin/rm INSTALL.*
