#	$NetBSD: Makefile,v 1.52 1999/06/07 00:53:09 thorpej Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM definition

SUBDIR=	csu libarch libbz2 libc libcompat libcrypt libcurses libedit libkvm \
	libl libm libntp libossaudio libpcap libposix libresolv librmt \
	librpcsvc libskey libterm libusb libutil libwrap liby libz

# XXX Keep telnet(1) and libtelnet from getting out of sync during the build
# XXX process.
.if (!exists(${.CURDIR}/../domestic/lib/libtelnet) && \
     !exists(${.CURDIR}/../crypto-us/lib/libtelnet)) || \
	defined(EXPORTABLE_SYSTEM) || make(obj) || \
	make(clean) || make(cleandir) || make(distclean)
SUBDIR+=	libtelnet
.endif

.include <bsd.subdir.mk>
