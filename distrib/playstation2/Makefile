#	$NetBSD: Makefile,v 1.4 2002/02/09 13:56:52 lukem Exp $

.include "../Makefile.inc"

.include <bsd.obj.mk>
.include <bsd.kernobj.mk>

SUBDIR=

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
MINIROOTDIR = ${.CURDIR}/../miniroot
MINIROOTOBJ != cd ${MINIROOTDIR} && ${PRINTOBJDIR}
MDSETIMAGE?=	mdsetimage

release:
	rm -f netbsd netbsd.gz
	cp ${KERNOBJDIR}/RAMDISK/netbsd .
	${MDSETIMAGE} netbsd ${MINIROOTOBJ}/miniroot.fs
	gzip -9 netbsd
	${IINST} netbsd.gz ${ITARGET}/.
	${.CURDIR}/../sets/makesums -t ${ITARGET} '*.gz'
.endif

.include <bsd.subdir.mk>
