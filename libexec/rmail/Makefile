#	$NetBSD: Makefile,v 1.9 1997/10/07 11:08:51 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

WARNS?=	1
PROG=	rmail
MAN=	rmail.8
# we need the link because of all the braindead UUCPs in the world that
# assume that rmail is in /bin -- sigh...
SYMLINKS=	/usr/libexec/rmail /bin/rmail

.include <bsd.prog.mk>
