# Makefile for uusched
# $Id: Makefile,v 1.1 1993/08/04 19:36:59 jtc Exp $

BINDIR=		$(bindir)

PROG=	uusched
SRCS=
NOMAN=

uusched:	uusched.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uusched.in > $(.TARGET)

.include <bsd.prog.mk>
