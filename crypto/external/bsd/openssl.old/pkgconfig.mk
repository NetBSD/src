# $NetBSD: pkgconfig.mk,v 1.1.1.1 2018/02/03 22:43:37 christos Exp $

.include <bsd.own.mk>

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
FILESBUILD_${pkg}.pc=yes

${pkg}.pc: ${.CURDIR}/../../mkpc
	CPPFLAGS=${CPPFLAGS:N-DOPENSSLDIR=*:N-DENGINESDIR=*:Q} CPP=${CPP:Q} ${HOST_SH} ${.ALLSRC} ${OPENSSLSRC}/crypto ${.TARGET} > ${.TARGET}
.endfor
