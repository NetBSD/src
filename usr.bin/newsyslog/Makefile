#	$NetBSD: Makefile,v 1.8 1997/10/24 09:00:50 lukem Exp $

PROG=	newsyslog

CPPFLAGS+= -DOSF
CPPFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CPPFLAGS+= -DPIDFILE=\"/var/run/syslog.pid\"
CPPFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CPPFLAGS+= -DCOMPRESS_POSTFIX=\".gz\"

BINOWN=	root

MAN=	newsyslog.8

.include <bsd.prog.mk>
