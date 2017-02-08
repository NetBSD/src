#	$NetBSD: Makefile,v 1.3 2017/02/08 16:21:24 christos Exp $

SUBDIR=rc.d

FILESDIR=               /usr/share/examples/nsd
FILESMODE=    644
FILES=	nsd.conf
FILESBUILD_nsd.conf=yes

nsd.conf: nsd.conf.sample.in __subst
CLEANFILES+= nsd.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
