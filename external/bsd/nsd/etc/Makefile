#	$NetBSD: Makefile,v 1.1.2.1 2017/04/21 16:52:03 bouyer Exp $

SUBDIR=rc.d

FILESDIR=               /usr/share/examples/nsd
FILESMODE=    644
FILES=	nsd.conf
FILESBUILD_nsd.conf=yes

nsd.conf: nsd.conf.sample.in __subst
CLEANFILES+= nsd.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
