#	$NetBSD: Makefile,v 1.20 2002/05/02 18:02:18 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		instkernel	# floppy-GENERIC
TARGETS+=	release

release: check_RELEASEDIR .WAIT README.files
.for i in floppy diskimage instkernel
	${RELEASE_INSTALL} ${.CURDIR}/README.files ${RELEASEDIR}/installation/$i
.endfor

.include <bsd.subdir.mk>
