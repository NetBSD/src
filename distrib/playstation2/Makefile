#	$NetBSD: Makefile,v 1.5 2002/03/05 21:23:31 thorpej Exp $

.include "../Makefile.inc"

.include <bsd.own.mk>
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
