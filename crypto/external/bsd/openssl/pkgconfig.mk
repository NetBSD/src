# $NetBSD: pkgconfig.mk,v 1.1 2013/01/18 18:09:55 christos Exp $

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
CLEANFILES+=${pkg}.pc

${pkg}.pc: ${.CURDIR}/../../mkpc
	${.ALLSRC} ${.TARGET} > ${.TARGET}
.endfor
