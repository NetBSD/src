#	$NetBSD: Makefile,v 1.3.2.2 2017/03/20 06:56:05 pgoyette Exp $

SUBDIR=rc.d

FILESDIR=               /usr/share/examples/nsd
FILESMODE=    644
FILES=	nsd.conf
FILESBUILD_nsd.conf=yes

nsd.conf: nsd.conf.sample.in __subst
CLEANFILES+= nsd.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
