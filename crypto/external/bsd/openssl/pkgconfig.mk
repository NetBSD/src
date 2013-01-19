# $NetBSD: pkgconfig.mk,v 1.3 2013/01/19 21:57:55 apb Exp $

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
FILESBUILD_${pkg}.pc=yes

${pkg}.pc: ${.CURDIR}/../../mkpc
	${.ALLSRC} ${.TARGET} > ${.TARGET}
.endfor
