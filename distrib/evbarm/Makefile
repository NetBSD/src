#	$NetBSD: Makefile,v 1.2 2002/03/29 01:52:45 thorpej Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf

SUBDIR=	gzboot

ITARGET=	${RELEASEDIR}/binary/gzimg
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

GZIMGS+=	gzimg_IQ80310_flash_0x00080000
GZIMGS+=	gzimg_IQ80321_flash_0xf0080000

.for img in ${GZIMGS}
GZIMGDIR_${img}!= cd ${.CURDIR}/gzboot/${img} && ${PRINTOBJDIR}
.endfor

release: check_RELEASEDIR
.for img in ${GZIMGS}
	${IINST} ${GZIMGDIR_${img}}/${img}.gz ${ITARGET}/.
.endfor

.include <bsd.subdir.mk>
