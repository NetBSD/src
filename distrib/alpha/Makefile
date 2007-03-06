#	$NetBSD: Makefile,v 1.22 2007/03/06 21:56:49 bouyer Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		instkernel cdroms # floppy-GENERIC
TARGETS+=	release

release: check_RELEASEDIR .WAIT README.files
.for i in floppy diskimage instkernel
	${RELEASE_INSTALL} ${.CURDIR}/README.files \
	    ${RELEASEDIR}/${MACHINE}/installation/$i
.endfor

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
