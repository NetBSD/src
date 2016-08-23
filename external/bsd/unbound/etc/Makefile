#	$NetBSD: Makefile,v 1.2 2016/08/23 15:04:29 christos Exp $

SUBDIR=rc.d

FILESDIR=               /etc/unbound
FILESMODE=    644
FILES=	unbound.conf
FILESBUILD_unbound.conf=yes

unbound.conf: example.conf.in __subst
CLEANFILES+= unbound.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
