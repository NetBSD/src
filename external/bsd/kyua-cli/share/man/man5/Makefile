# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:23 tls Exp $

.include <bsd.init.mk>

.PATH:	${SRCDIR}/doc

.for name in kyua.conf.5 kyuafile.5
MAN+=	${name}
${name}: ${name}.in
	name=${name}; ${BUILD_MANPAGE}
.endfor

.include <bsd.man.mk>
