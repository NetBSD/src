#	$NetBSD: Makefile,v 1.21 2003/01/03 15:34:31 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		instkernel	# floppy-GENERIC
TARGETS+=	release

release: check_RELEASEDIR .WAIT README.files
.for i in floppy diskimage instkernel
	${RELEASE_INSTALL} ${.CURDIR}/README.files \
	    ${RELEASEDIR}/${MACHINE}/installation/$i
.endfor

.include <bsd.subdir.mk>
