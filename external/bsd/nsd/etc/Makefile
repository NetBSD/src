#	$NetBSD: Makefile,v 1.2 2017/02/07 22:34:37 christos Exp $

SUBDIR=rc.d

FILESDIR=               /usr/share/examples/nsd
FILESMODE=    644
FILES=	nsd.conf

nsd.conf: nsd.conf.sample.in __subst
CLEANFILES+= nsd.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
