#	$NetBSD: Makefile,v 1.13 2002/03/06 13:31:04 lukem Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

SUBDIR=	instkernel #floppy-GENERIC

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

CDHDTAPEOBJ!=	cd ${.CURDIR}/instkernel/cd-hd-tape && ${PRINTOBJDIR}
FDSETOBJ!=	cd ${.CURDIR}/instkernel/fdset && ${PRINTOBJDIR}
INSTKERNOBJ!=	cd ${.CURDIR}/instkernel/instkernel && ${PRINTOBJDIR}
GENERICOBJ!=	cd ${.CURDIR}/floppy-GENERIC && ${PRINTOBJDIR}

release: check_RELEASEDIR
.for i in floppy diskimage instkernel tapeimage
	${IINST} ${.CURDIR}/README.files ${ITARGET}/$i/.
.endfor
	${IINST} ${CDHDTAPEOBJ}/cdhdtape ${ITARGET}/diskimage/.
	${IINST} ${FDSETOBJ}/disk1of2 ${ITARGET}/floppy/.
	${IINST} ${FDSETOBJ}/disk2of2 ${ITARGET}/floppy/.
	${IINST} ${INSTKERNOBJ}/netbsd.gz ${ITARGET}/instkernel/.
	#${IINST} ${GENERICOBJ}/GENERIC.fs ${ITARGET}/misc/.

.include <bsd.subdir.mk>
