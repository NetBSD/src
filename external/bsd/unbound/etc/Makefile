#	$NetBSD: Makefile,v 1.2.2.3 2017/03/20 06:56:27 pgoyette Exp $

SUBDIR=rc.d

FILESDIR=               /usr/share/examples/unbound
FILESMODE=    644
FILES=	unbound.conf
FILESBUILD_unbound.conf=yes

unbound.conf: example.conf.in __subst
CLEANFILES+= unbound.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
