#	$NetBSD: Makefile,v 1.3 2017/02/07 22:34:37 christos Exp $

SUBDIR=rc.d

FILESDIR=               /usr/share/examples/unbound
FILESMODE=    644
FILES=	unbound.conf
FILESBUILD_unbound.conf=yes

unbound.conf: example.conf.in __subst
CLEANFILES+= unbound.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
