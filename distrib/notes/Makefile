#	$NetBSD: Makefile,v 1.1.1.1 1994/09/17 19:02:59 cgd Exp $

M?=	i386

TARG=	INSTALL.$M
SRC=	${.CURDIR}/INSTALL
DEP=	${SRC} ${.CURDIR}/mirrors ${.CURDIR}/$M/whatis ${.CURDIR}/$M/contents \
	${.CURDIR}/$M/hardware ${.CURDIR}/$M/xfer ${.CURDIR}/$M/prep \
	${.CURDIR}/$M/install ${.CURDIR}/$M/upgrade ${.CURDIR}/donations \
	${.CURDIR}/legal.common ${.CURDIR}/$M/legal

${TARG}: ${DEP}
	cpp -I${.CURDIR}/$M -DMACHINE=$M -C -P ${SRC} > $@

clean cleandir:
	/bin/rm INSTALL.*
