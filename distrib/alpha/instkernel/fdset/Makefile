#	$NetBSD: Makefile,v 1.10 2002/05/02 18:02:20 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=	disk
FLOPPYSIZE=	2880
FLOPPYFILES=	boot netbsd
FLOPPYMAX=	2

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd.INSTALL.gz
FLOPPY_BOOT=	${MDEC}/ustarboot
FLOPPYINSTBOOT=\
	"${INSTALLBOOT} -m ${MACHINE} -t raw -v -b 17 @IMAGE@ ${FLOPPY_BOOT}"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

release: check_RELEASEDIR .WAIT ${FLOPPYBASE}1.fs
	${RELEASE_INSTALL} ${FLOPPYBASE}1.fs \
	    ${RELEASEDIR}/installation/floppy/disk1of2
	${RELEASE_INSTALL} ${FLOPPYBASE}2.fs \
	    ${RELEASEDIR}/installation/floppy/disk2of2

.include <bsd.prog.mk>
