#	$NetBSD: Makefile,v 1.16 2003/01/03 15:34:35 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT ramdisk-kernel .WAIT floppy
TARGETS+=	release

release: check_RELEASEDIR .WAIT ${DESTDIR}/usr/mdec/boot
	${RELEASE_INSTALL} ${DESTDIR}/usr/mdec/boot \
	    ${RELEASEDIR}/${MACHINE}/installation/netboot

.include <bsd.subdir.mk>
