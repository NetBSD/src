#	$Id: Makefile,v 1.3 1993/05/28 09:08:28 cgd Exp $

PROG=	newsyslog

CFLAGS+= -DOSF
CFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CFLAGS+= -DPIDFILE=\"/var/run/syslog.pid\"
CFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CFLAGS+= -DCOMPRESS_POSTFIX=\".z\"

BINOWN=	root

MAN5=	newsyslog.conf.0
MAN8=	newsyslog.0

CLEANFILES+= newsyslog.conf.5

newsyslog.conf.5:
	ln -s ${.CURDIR}/newsyslog.8 $@

.include <bsd.prog.mk>
