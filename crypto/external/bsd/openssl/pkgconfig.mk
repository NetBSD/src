# $NetBSD: pkgconfig.mk,v 1.4.4.3 2014/08/19 23:45:25 tls Exp $

.include <bsd.own.mk>

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
FILESBUILD_${pkg}.pc=yes

${pkg}.pc: ${.CURDIR}/../../mkpc
	CPPFLAGS=${CPPFLAGS:N-DOPENSSLDIR=*:N-DENGINESDIR=*:Q} CPP=${CPP:Q} ${HOST_SH} ${.ALLSRC} ${OPENSSLSRC}/crypto ${.TARGET} > ${.TARGET}
.endfor
