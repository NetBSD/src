# $NetBSD: Makefile,v 1.15 2002/05/09 20:22:26 jdolecek Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT kernels stand
TARGETS+=	release

release: check_RELEASEDIR
	${RELEASE_INSTALL} ${DISTRIBDIR}/notes/acorn32/prep.RISCOS \
		    ${RELEASEDIR}/
	uudecode < ${.CURDIR}/gzip_ff8.bin.uue && \
		mv -f gzip_ff8.bin ${RELEASEDIR}/installation/misc/

.include <bsd.subdir.mk>
