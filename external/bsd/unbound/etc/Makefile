#	$NetBSD: Makefile,v 1.2.4.1 2017/04/21 16:52:40 bouyer Exp $

SUBDIR=rc.d

FILESDIR=               /usr/share/examples/unbound
FILESMODE=    644
FILES=	unbound.conf
FILESBUILD_unbound.conf=yes

unbound.conf: example.conf.in __subst
CLEANFILES+= unbound.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
