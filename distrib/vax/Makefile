#	$NetBSD: Makefile,v 1.9 2002/02/09 13:56:53 lukem Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

SUBDIR= ramdisk ramdisk-kernel floppy #inst tk50 rx50 rx33

.include <bsd.subdir.mk>

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
INSTKERNOBJ != cd ${.CURDIR}/ramdisk-kernel && ${PRINTOBJDIR}
FLOPPYOBJ != cd ${.CURDIR}/floppy && ${PRINTOBJDIR}

release:
	${IINST} ${DESTDIR}/usr/mdec/boot.mop ${ITARGET}/netboot/
	${IINST} ${INSTKERNOBJ}/netbsd.INSTALL.gz ${ITARGET}/netboot/netbsd.ram.gz
	${IINST} ${FLOPPYOBJ}/boot.fs ${ITARGET}/bootfs/
.endif
