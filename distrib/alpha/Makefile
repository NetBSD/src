#	$NetBSD: Makefile,v 1.22.12.1 2008/06/29 08:25:12 mjf Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		instkernel cdroms # floppy-GENERIC
TARGETS+=	release

release: check_RELEASEDIR .WAIT README.files
.for i in floppy diskimage instkernel
	${RELEASE_INSTALL} ${.CURDIR}/README.files \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/$i
.endfor

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
