#	$NetBSD: Makefile,v 1.7.2.1 1997/11/08 10:37:47 lukem Exp $

PROG=	newsyslog

CPPFLAGS+= -DOSF
CPPFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CPPFLAGS+= -DPIDFILE=\"/var/run/syslog.pid\"
CPPFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CPPFLAGS+= -DCOMPRESS_POSTFIX=\".gz\"

BINOWN=	root

MAN=	newsyslog.8

.include <bsd.prog.mk>
