#	$NetBSD: Makefile,v 1.17 2002/04/26 17:30:00 matt Exp $

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
	${RELINSTALL} ${CDHDTAPEOBJ}/cdhdtape ${ITARGET}/diskimage/.
	${RELINSTALL} ${FDSETOBJ}/disk1of2 ${ITARGET}/floppy/.
	${RELINSTALL} ${FDSETOBJ}/disk2of2 ${ITARGET}/floppy/.
	${RELINSTALL} ${INSTKERNOBJ}/netbsd.INSTALL.gz ${ITARGET}/instkernel/netbsd.gz
#	${RELINSTALL} ${GENERICOBJ}/GENERIC.fs ${ITARGET}/misc/.

.include <bsd.subdir.mk>
