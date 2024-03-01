# $NetBSD: pkgconfig.mk,v 1.1.8.2 2024/03/01 11:43:31 martin Exp $

.include <bsd.own.mk>

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
FILESBUILD_${pkg}.pc=yes

${pkg}.pc: ${.CURDIR}/../mkpc
	CPPFLAGS=${CPPFLAGS:Q} CPP=${CPP:Q} ${HOST_SH} ${.ALLSRC} ${LIBUVDIR}/include > ${.TARGET}
.endfor
