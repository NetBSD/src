#	$NetBSD: Makefile,v 1.4 1997/06/17 08:00:13 mikel Exp $

M?=	${MACHINE}
V!=	/bin/sh ${.CURDIR}/../../sys/conf/osrelease.sh

TARG=	INSTALL.$M
SRC=	${.CURDIR}/INSTALL
DEP=	${SRC} ${.CURDIR}/mirrors ${.CURDIR}/$M/whatis ${.CURDIR}/$M/contents \
	${.CURDIR}/$M/hardware ${.CURDIR}/$M/xfer ${.CURDIR}/$M/prep \
	${.CURDIR}/$M/install ${.CURDIR}/$M/upgrade ${.CURDIR}/donations \
	${.CURDIR}/legal.common ${.CURDIR}/$M/legal

${TARG}: ${DEP}
	cpp -I${.CURDIR}/$M -D_MACH=$M -D_VER=$V -Uunix -U$M -C -P ${SRC} > $@

clean cleandir:
	/bin/rm INSTALL.*
