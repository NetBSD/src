#	$NetBSD: Makefile,v 1.6 1998/01/09 08:10:53 perry Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 7/2/95

BINDIR=	/usr/libexec
PROG=	smrsh
MAN=	smrsh.8
CPPFLAGS+=-I${.CURDIR}/../src -DNDBM -DNEWDB
CPPFLAGS+=-DCMDDIR=\"/usr/libexec/sm.bin\"

.include <bsd.prog.mk>
