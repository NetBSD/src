# $NetBSD: pkgconfig.mk,v 1.2 2013/01/19 21:05:46 christos Exp $

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
CLEANFILES+=${pkg}.pc

realall: ${pkg}.pc
${pkg}.pc: ${.CURDIR}/../../mkpc
	${.ALLSRC} ${.TARGET} > ${.TARGET}
.endfor
