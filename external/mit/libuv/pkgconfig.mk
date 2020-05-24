# $NetBSD: pkgconfig.mk,v 1.1 2020/05/24 19:28:10 christos Exp $

.include <bsd.own.mk>

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
FILESBUILD_${pkg}.pc=yes

${pkg}.pc: ${.CURDIR}/../mkpc
	CPPFLAGS=${CPPFLAGS:Q} CPP=${CPP:Q} ${HOST_SH} ${.ALLSRC} ${LIBUVDIR}/include > ${.TARGET}
.endfor
