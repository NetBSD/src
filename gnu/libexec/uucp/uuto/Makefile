# Makefile for uuto

BINDIR=		$(bindir)

PROG=		uuto
SRCS=
NOMAN=

uuto:	uuto.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uuto.in > $(.TARGET)

.include <bsd.prog.mk>
