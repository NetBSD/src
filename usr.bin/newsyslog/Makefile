#	$Id: Makefile,v 1.5 1993/12/22 04:38:45 cgd Exp $

PROG=	newsyslog

CFLAGS+= -DOSF
CFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CFLAGS+= -DPIDFILE=\"/var/run/syslog.pid\"
CFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CFLAGS+= -DCOMPRESS_POSTFIX=\".gz\"

BINOWN=	root

MAN8=	newsyslog.0

.include <bsd.prog.mk>
