#	$NetBSD: Makefile,v 1.4.2.1 1997/11/17 02:37:12 thorpej Exp $

M?=	${MACHINE}
V!=	/bin/sh ${.CURDIR}/../../sys/conf/osrelease.sh
V_S!=	/bin/sh ${.CURDIR}/../../sys/conf/osrelease.sh -s

TARG=	INSTALL.$M
SRC=	${.CURDIR}/INSTALL
DEP=	${SRC} ${.CURDIR}/mirrors ${.CURDIR}/$M/whatis ${.CURDIR}/$M/contents \
	${.CURDIR}/$M/hardware ${.CURDIR}/$M/xfer ${.CURDIR}/$M/prep \
	${.CURDIR}/$M/install ${.CURDIR}/$M/upgrade ${.CURDIR}/donations \
	${.CURDIR}/legal.common ${.CURDIR}/$M/legal

${TARG}: ${DEP}
	cpp -I${.CURDIR}/$M -D_MACH=$M -D_VER=$V -Uunix -U$M -C -P ${SRC} \
		| sed 's/_SVER_S/'${V_S}'/g ' > $@

clean cleandir:
	/bin/rm INSTALL.*
