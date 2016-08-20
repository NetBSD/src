#	$NetBSD: Makefile,v 1.1 2016/08/20 11:20:31 christos Exp $

SUBDIR=rc.d

FILESDIR=               /etc/unbound
FILESMODE=    644
FILES=	unbound.conf

unbound.conf: example.conf.in __subst
CLEANFILES+= unbound.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
