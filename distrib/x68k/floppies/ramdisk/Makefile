#	$NetBSD: Makefile,v 1.18 2000/09/12 16:42:50 minoura Exp $

TOP=		${.CURDIR}/..
COMMONDIR=	${TOP}/ramdisk-common

IMAGE=		ramdisk.fs

AUXTARGETS=	install.sh upgrade.sh start.sh
AUXDEPENDS=	${AUXTARGETS} \
		${COMMONDIR}/termcap.vt ${COMMONDIR}/disktab.preinstall \
		${COMMONDIR}/installboot.sh
AUXCLEAN=	${AUXTARGETS}

CBIN=		ramdiskbin

RAMDISKSPT=	15
RAMDISKCYLS=	80
RAMDISKHEADS=	2
RAMDISKCPG=	80
RAMDISKBPI=	5120

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
