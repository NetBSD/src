#	$NetBSD: Makefile.inc,v 1.14 2002/05/30 05:46:16 thorpej Exp $

.include <bsd.own.mk>

LIBUNIXDIR!=cd $(.CURDIR)/../libunix && ${PRINTOBJDIR}
LIBUNIX=${LIBUNIXDIR}/libunix.a

LIBUUCONFDIR!=cd $(.CURDIR)/../libuuconf && ${PRINTOBJDIR}
LIBUUCONF=${LIBUUCONFDIR}/libuuconf.a

LIBUUCPDIR!=cd $(.CURDIR)/../libuucp && ${PRINTOBJDIR}
LIBUUCP=${LIBUUCPDIR}/libuucp.a

CPPFLAGS+=-DPOSIX_HEADERS
WARNS=		1
CWARNFLAGS+=	-Wno-format-zero-length
VERSION=	1.06.1
owner=		uucp
bindir=		/usr/bin
sbindir=	/usr/libexec/uucp

# The directory to look in for new style configuration files (when
# using HAVE_TAYLOR_CONFIG).
newconfigdir=	/etc/uucp

# The directory to look in for BNU (when using HAVE_BNU_CONFIG) or
# V2 (when using HAVE_V2_CONFIG) style configuration files.
oldconfigdir=	/etc/uucp
