#	$NetBSD: Makefile,v 1.9 1998/07/29 01:14:16 simonb Exp $

PROG=	newsyslog

CPPFLAGS+= -DOSF
CPPFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CPPFLAGS+= -DPIDFILE=\"/var/run/syslog.pid\"
CPPFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CPPFLAGS+= -DCOMPRESS_POSTFIX=\".gz\"

MAN=	newsyslog.8

.include <bsd.prog.mk>
