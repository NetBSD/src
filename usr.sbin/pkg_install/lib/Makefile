# $NetBSD: Makefile,v 1.22 2003/09/23 14:23:53 wiz Exp $
# Original from FreeBSD, no rcs id.

NOLINT=		# defined
NOMAN=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB+=		install
SRCS+=		file.c ftpio.c global.c lpkg.c pen.c pkgdb.c \
		plist.c str.c version.c path.c fexec.c

# only needed during build - prevent installation of library
libinstall::

version.o: version.h version.c

.include <bsd.lib.mk>
