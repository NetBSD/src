#	$NetBSD: Makefile,v 1.1.1.1 2000/02/08 13:45:34 tron Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/src

PROG=		sendmail

CPPFLAGS+=	${DBMDEF} -DMAP_REGEX -DTCPWRAPPERS
LDADD+=		-lwrap
DPADD+=		${LIBWRAP}

SRCS=		alias.c arpadate.c clock.c collect.c conf.c control.c \
		convtime.c daemon.c deliver.c domain.c envelope.c err.c \
		headers.c macro.c main.c map.c mci.c mime.c parseaddr.c \
		queue.c readcf.c recipient.c safefile.c savemail.c snprintf.c \
		srvrsmtp.c stab.c stats.c sysexits.c trace.c udb.c usersmtp.c \
		util.c version.c
MAN=		aliases.5 mailq.1 newaliases.1 sendmail.8

BINDIR= /usr/libexec/sendmail
BINOWN= root
BINGRP= kmem
BINMODE=6555
.if ${MKSHARE} != "no"
FILES=sendmail.hf
FILESDIR=/usr/share/misc
.endif

afterinstall:
	${INSTALL} ${INSTPRIV} -c -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
	    ${DESTDIR}/var/log/sendmail.st


.include <bsd.prog.mk>
