#	$NetBSD: Makefile,v 1.7 1996/09/27 01:56:55 thorpej Exp $

PROG=	newsyslog

CFLAGS+= -DOSF
CFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CFLAGS+= -DPIDFILE=\"/var/run/syslog.pid\"
CFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CFLAGS+= -DCOMPRESS_POSTFIX=\".gz\"

BINOWN=	root

MAN=	newsyslog.8

.include <bsd.prog.mk>
