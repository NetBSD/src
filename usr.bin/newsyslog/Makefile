#	$NetBSD: Makefile,v 1.11.4.1 1999/12/27 18:37:07 wrstuden Exp $

PROG=	newsyslog

CPPFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CPPFLAGS+= -DPIDFILE=\"/var/run/syslogd.pid\"
CPPFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CPPFLAGS+= -DCOMPRESS_POSTFIX=\".gz\"

MAN=	newsyslog.8
MLINKS+=newsyslog.8 newsyslog.conf.5

.include <bsd.prog.mk>
