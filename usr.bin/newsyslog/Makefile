#	$NetBSD: Makefile,v 1.10 1998/07/29 01:31:50 simonb Exp $

PROG=	newsyslog

CPPFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CPPFLAGS+= -DPIDFILE=\"/var/run/syslog.pid\"
CPPFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CPPFLAGS+= -DCOMPRESS_POSTFIX=\".gz\"

MAN=	newsyslog.8

.include <bsd.prog.mk>
