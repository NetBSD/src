# $NetBSD: pkgconfig.mk,v 1.5 2014/04/07 17:40:49 apb Exp $

.include <bsd.own.mk>

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
FILESBUILD_${pkg}.pc=yes

${pkg}.pc: ${.CURDIR}/../../mkpc
	${HOST_SH} ${.ALLSRC} ${OPENSSLSRC}/crypto ${.TARGET} > ${.TARGET}
.endfor
