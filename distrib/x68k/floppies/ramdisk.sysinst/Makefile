#	$NetBSD: Makefile,v 1.11 2000/09/12 17:07:44 minoura Exp $

TOP=		${.CURDIR}/..
COMMONDIR=	${TOP}/ramdisk-common

IMAGE=		ramdisk.fs

AUXTARGETS=	passwd
AUXDEPENDS= 	${AUXTARGETS} dot.profile dot.hdprofile \
		${COMMONDIR}/termcap.vt ${COMMONDIR}/disktab.preinstall \
		${COMMONDIR}/installboot.sh
AUXCLEAN=	${AUXTARGETS}

CBIN=		ramdiskbin

RAMDISKSPT=	22
RAMDISKCYLS=	50
RAMDISKHEADS=	2
RAMDISKCPG=	56
RAMDISKBPI=	4096

passwd:
	-/bin/mkdir ${.OBJDIR}/etc
	/bin/cp -pf ${.CURDIR}/../../../../etc/master.passwd ${.OBJDIR}/etc
	pwd_mkdb -p -d ${.OBJDIR} ${.OBJDIR}/etc/master.passwd
	/bin/cp -pf ${.OBJDIR}/etc/passwd ${.OBJDIR}

.include "${COMMONDIR}/Makefile.inc"

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.sys.mk>
