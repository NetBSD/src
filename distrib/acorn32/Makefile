# $NetBSD: Makefile,v 1.14 2002/05/02 18:02:17 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT kernels
TARGETS+=	release

release: check_RELEASEDIR
	cd ${NETBSDSRCDIR}/sys/arch/acorn32/stand && \
		./export-BtNetBSD && \
		rm -f ${RELEASEDIR}/installation/misc/BtNetBSD.tgz && \
		gzip -9 -c BtNetBSD.tar > \
		    ${RELEASEDIR}/installation/misc/BtNetBSD.tgz && \
		rm -f BtNetBSD.tar && \
		mv -f BtNetBSD.zip ${RELEASEDIR}/installation/misc && \
		${RELEASE_INSTALL} ${DISTRIBDIR}/notes/acorn32/prep.RISCOS \
		    ${RELEASEDIR}/
	cd ${.CURDIR} && \
		rm -f ${RELEASEDIR}/installation/misc/gzip_ff8.bin && \
		uudecode < gzip_ff8.bin.uue && \
		mv -f gzip_ff8.bin ${RELEASEDIR}/installation/misc/

.include <bsd.subdir.mk>
