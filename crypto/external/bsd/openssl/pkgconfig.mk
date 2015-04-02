# $NetBSD: pkgconfig.mk,v 1.5.6.4 2015/04/02 17:58:37 snj Exp $

.include <bsd.own.mk>

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
FILESBUILD_${pkg}.pc=yes

${pkg}.pc: ${.CURDIR}/../../mkpc
	CPPFLAGS=${CPPFLAGS:N-DOPENSSLDIR=*:N-DENGINESDIR=*:Q} CPP=${CPP:Q} ${HOST_SH} ${.ALLSRC} ${OPENSSLSRC}/crypto ${.TARGET} > ${.TARGET}
.endfor
