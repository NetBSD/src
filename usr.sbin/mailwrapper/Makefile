#	$NetBSD: Makefile,v 1.3 1999/02/20 22:10:07 thorpej Exp $

PROG=	mailwrapper
MAN=	mailwrapper.8

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

SYMLINKS= 	/usr/sbin/mailwrapper /usr/sbin/sendmail  \
		/usr/sbin/mailwrapper /usr/bin/newaliases \
		/usr/sbin/mailwrapper /usr/bin/mailq

.include <bsd.prog.mk>
