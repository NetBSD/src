#	$NetBSD: Makefile,v 1.19 2000/09/12 17:07:44 minoura Exp $

TOP=		${.CURDIR}/..
COMMONDIR=	${TOP}/ramdisk-common

IMAGE=		ramdisk.fs

AUXTARGETS=	install.sh upgrade.sh start.sh
AUXDEPENDS=	${AUXTARGETS} \
		${COMMONDIR}/termcap.vt ${COMMONDIR}/disktab.preinstall \
		${COMMONDIR}/installboot.sh
AUXCLEAN=	${AUXTARGETS}

CBIN=		ramdiskbin

RAMDISKSPT=	23
RAMDISKCYLS=	50
RAMDISKHEADS=	2
RAMDISKCPG=	80
RAMDISKBPI=	4096

install.sh: install.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

upgrade.sh: upgrade.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

start.sh: start.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

.include "${COMMONDIR}/Makefile.inc"

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.sys.mk>
