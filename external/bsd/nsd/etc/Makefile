#	$NetBSD: Makefile,v 1.1 2017/01/07 19:51:21 christos Exp $

SUBDIR=rc.d

FILESDIR=               /etc/nsd
FILESMODE=    644
FILES=	nsd.conf

nsd.conf: nsd.conf.sample.in __subst
CLEANFILES+= nsd.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
