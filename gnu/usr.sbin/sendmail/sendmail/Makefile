#	$NetBSD: Makefile,v 1.5 2000/07/07 04:26:59 cgd Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/sendmail

BINDIR=		/usr/libexec/sendmail
PROG=		sendmail

BINOWN=		root
BINMODE=	4555

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil; ${MAKE} print-objdir

LDADD+=		-lwrap -L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBWRAP} ${LIBSMUTILDIR}/libsmutil.a

SRCS=		alias.c arpadate.c bf_torek.c clock.c collect.c \
		conf.c control.c convtime.c daemon.c deliver.c \
		domain.c envelope.c err.c headers.c macro.c main.c \
		map.c mci.c milter.c mime.c parseaddr.c queue.c \
		readcf.c recipient.c savemail.c sfsasl.c shmticklib.c \
		srvrsmtp.c stab.c stats.c sysexits.c timers.c trace.c \
		udb.c usersmtp.c util.c version.c

MAN=		aliases.5 mailq.1 newaliases.1 sendmail.8

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${DIST}/sendmail/sendmail/helpfile ${DESTDIR}/etc/mail

.include <bsd.prog.mk>
