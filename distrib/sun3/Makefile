#	$NetBSD: Makefile,v 1.7 2002/05/17 17:52:29 lukem Exp $

SUBDIR=		ramdisk miniroot
TARGETS+=	release

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

release: check_RELEASEDIR .WAIT \
	    ${DESTDIR}/usr/mdec/netboot ${DESTDIR}/usr/mdec/tapeboot \
	    MakeBootTape MakeInstTape
	${RELEASE_INSTALL} \
	    ${DESTDIR}/usr/mdec/netboot \
	    ${RELEASEDIR}/installation/netboot
	${RELEASE_INSTALL} \
	    ${DESTDIR}/usr/mdec/tapeboot \
	    ${RELEASEDIR}/installation/tapeimage
	${RELEASE_INSTALL} \
	    ${.CURDIR}/MakeBootTape ${.CURDIR}/MakeInstTape \
	    ${RELEASEDIR}/installation/tapeimage

.include <bsd.subdir.mk>
