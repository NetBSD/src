#	$NetBSD: Makefile,v 1.12 1999/12/07 11:28:13 ad Exp $

PROG=	newsyslog

CPPFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CPPFLAGS+= -DPIDFILE=\"/var/run/syslogd.pid\"
CPPFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CPPFLAGS+= -DCOMPRESS_POSTFIX=\".gz\"

MAN=	newsyslog.8
MLINKS+=newsyslog.8 newsyslog.conf.5

.include <bsd.prog.mk>
