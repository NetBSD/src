# $NetBSD: pkgconfig.mk,v 1.3.2.2 2013/01/23 00:04:08 yamt Exp $

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
FILESBUILD_${pkg}.pc=yes

${pkg}.pc: ${.CURDIR}/../../mkpc
	${.ALLSRC} ${.TARGET} > ${.TARGET}
.endfor
