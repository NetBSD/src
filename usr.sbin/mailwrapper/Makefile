#	$NetBSD: Makefile,v 1.5 2000/07/26 16:35:34 itojun Exp $

PROG=	mailwrapper
MAN=	mailwrapper.8 mailer.conf.5

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

SYMLINKS= 	/usr/sbin/mailwrapper /usr/sbin/sendmail  \
		/usr/sbin/mailwrapper /usr/bin/newaliases \
		/usr/sbin/mailwrapper /usr/bin/mailq \
		/usr/sbin/mailwrapper /usr/bin/hoststat \
		/usr/sbin/mailwrapper /usr/bin/purgestat

.include <bsd.prog.mk>
