#	$Id: Makefile,v 1.1 1993/05/21 14:43:59 cgd Exp $

PROG=	newsyslog

CFLAGS+= -DOSF
CFLAGS+= -DCONF=\"/etc/newsyslog.conf\"
CFLAGS+= -DPIDFILE=\"/var/run/syslog.pid\"
CFLAGS+= -DCOMPRESS=\"/usr/bin/gzip\"

MAN5=	newsyslog.conf.0
MAN8=	newsyslog.0

CLEANFILES+= newsyslog.conf.5

newsyslog.conf.5:
	ln -s ${.CURDIR}/newsyslog.8 $@

.include <bsd.prog.mk>
