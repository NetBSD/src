#	$NetBSD: Makefile,v 1.12 2005/03/15 02:14:20 atatat Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/sendmail

BINDIR=		/usr/libexec/sendmail
PROG=		sendmail

OBINGRP:=	${BINGRP}
.if !defined(SENDMAIL_SUIDROOT) || ${SENDMAIL_SUIDROOT} != "yes"
BINGRP=		smmsp
BINMODE=	2555
.else
BINOWN=		root
BINMODE=	4555
.endif

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil && ${PRINTOBJDIR}
LIBSMDIR!=	cd ${.CURDIR}/../libsm && ${PRINTOBJDIR}

LDADD+=		-lwrap -L${LIBSMUTILDIR} -lsmutil -L${LIBSMDIR} -lsm -lutil
DPADD+=		${LIBWRAP} ${LIBSMUTILDIR}/libsmutil.a ${LIBSMDIR}/libsm.a \
		${LIBUTIL}

SRCS=		alias.c arpadate.c bf.c collect.c conf.c control.c \
		convtime.c daemon.c deliver.c domain.c envelope.c err.c \
		headers.c macro.c main.c map.c mci.c milter.c mime.c \
		parseaddr.c queue.c ratectrl.c readcf.c recipient.c sasl.c \
		savemail.c sfsasl.c shmticklib.c sm_resolve.c srvrsmtp.c \
		stab.c stats.c sysexits.c timers.c tls.c trace.c udb.c \
		usersmtp.c util.c version.c

MAN=		aliases.5 mailq.1 newaliases.1 sendmail.8

FILES=		${DIST}/sendmail/sendmail/helpfile
FILESDIR=	/etc/mail
FILESOWN=	${BINOWN}
FILESGRP=	${OBINGRP}
FILESMODE=	${NONBINMODE}

.include <bsd.prog.mk>
