#	$NetBSD: Makefile,v 1.4.2.3 1998/11/07 04:08:25 cgd Exp $

NOPROG=	notreally

M?=	${MACHINE}
V!=	/bin/sh ${.CURDIR}/../../sys/conf/osrelease.sh
V_S!=	/bin/sh ${.CURDIR}/../../sys/conf/osrelease.sh -s

TARG=	INSTALL.$M
SRC=	${.CURDIR}/INSTALL
DEP=	${SRC} ${.CURDIR}/mirrors ${.CURDIR}/$M/whatis ${.CURDIR}/$M/contents \
	${.CURDIR}/$M/hardware ${.CURDIR}/$M/xfer ${.CURDIR}/$M/prep \
	${.CURDIR}/$M/install ${.CURDIR}/$M/upgrade ${.CURDIR}/donations \
	${.CURDIR}/legal.common ${.CURDIR}/$M/legal

CLEANFILES+=${TARG}

all:	${TARG}

${TARG}: ${DEP}
	cpp -I${.CURDIR}/$M -D_MACH=$M -D_VER=$V -Uunix -U$M -C -P ${SRC} \
		| sed -e 's/_SVER_S/'${V_S}'/g ' \
		      -e 's/[\t ]*[$$]NetBSD:..*[$$]//g' > $@

.include <bsd.prog.mk>
