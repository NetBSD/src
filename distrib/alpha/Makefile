#	$NetBSD: Makefile,v 1.19 2002/05/02 13:04:48 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

SUBDIR=		instkernel	# floppy-GENERIC

ITARGET=	${RELEASEDIR}/installation

CDHDTAPEOBJ!=	cd ${.CURDIR}/instkernel/cd-hd-tape && ${PRINTOBJDIR}
FDSETOBJ!=	cd ${.CURDIR}/instkernel/fdset && ${PRINTOBJDIR}
INSTKERNOBJ!=	cd ${.CURDIR}/instkernel/instkernel && ${PRINTOBJDIR}
GENERICOBJ!=	cd ${.CURDIR}/floppy-GENERIC && ${PRINTOBJDIR}

release: check_RELEASEDIR
.for i in floppy diskimage instkernel tapeimage
	${RELINSTALL} ${.CURDIR}/README.files ${ITARGET}/$i/.
.endfor
	${RELINSTALL} ${CDHDTAPEOBJ}/cdhdtape1.fs ${ITARGET}/diskimage/cdhdtape
	${RELINSTALL} ${FDSETOBJ}/disk1.fs ${ITARGET}/floppy/disk1of2
	${RELINSTALL} ${FDSETOBJ}/disk2.fs ${ITARGET}/floppy/disk2of2
	${RELINSTALL} ${INSTKERNOBJ}/netbsd.INSTALL.gz ${ITARGET}/instkernel/netbsd.gz
#	${RELINSTALL} ${GENERICOBJ}/GENERIC.fs ${ITARGET}/misc/.

.include <bsd.subdir.mk>
