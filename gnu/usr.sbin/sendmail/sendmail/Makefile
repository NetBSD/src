#	$NetBSD: Makefile,v 1.2 2000/02/26 18:41:43 mycroft Exp $

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
FILESGRP=wheel
FILESDIR=/usr/share/misc
.endif

.include <bsd.prog.mk>
