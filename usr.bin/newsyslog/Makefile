#	$NetBSD: Makefile,v 1.11 1999/06/09 11:39:46 tron Exp $

PROG=	newsyslog

CPPFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CPPFLAGS+= -DPIDFILE=\"/var/run/syslogd.pid\"
CPPFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CPPFLAGS+= -DCOMPRESS_POSTFIX=\".gz\"

MAN=	newsyslog.8

.include <bsd.prog.mk>
