#	$Id: Makefile,v 1.4 1993/06/29 15:37:37 mycroft Exp $

PROG=	newsyslog

CFLAGS+= -DOSF
CFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CFLAGS+= -DPIDFILE=\"/var/run/syslog.pid\"
CFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"
CFLAGS+= -DCOMPRESS_POSTFIX=\".gz\"

BINOWN=	root

MAN5=	newsyslog.conf.0
MAN8=	newsyslog.0

CLEANFILES+= newsyslog.conf.5

newsyslog.conf.5:
	ln -s ${.CURDIR}/newsyslog.8 $@

.include <bsd.prog.mk>
