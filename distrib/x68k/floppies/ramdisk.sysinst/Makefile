#	$NetBSD: Makefile,v 1.14 2001/12/30 04:08:28 minoura Exp $

TOP=		${.CURDIR}/..
COMMONDIR=	${TOP}/ramdisk-common

IMAGE=		ramdisk.fs

AUXTARGETS=	passwd
AUXDEPENDS= 	${AUXTARGETS} dot.profile dot.hdprofile \
		${COMMONDIR}/termcap.vt ${COMMONDIR}/disktab.preinstall
AUXCLEAN=	${AUXTARGETS}

CBIN=		ramdiskbin

RAMDISKSPT=	15
RAMDISKCYLS=	85
RAMDISKHEADS=	2
RAMDISKCPG=	96
RAMDISKBPI=	4096

passwd:
	-/bin/mkdir ${.OBJDIR}/etc
	/bin/cp -pf ${.CURDIR}/../../../../etc/master.passwd ${.OBJDIR}/etc
	pwd_mkdb -p -d ${.OBJDIR} ${.OBJDIR}/etc/master.passwd
	/bin/cp -pf ${.OBJDIR}/etc/passwd ${.OBJDIR}

includes:

.include "${COMMONDIR}/Makefile.inc"

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.sys.mk>
.include <bsd.prog.mk>
