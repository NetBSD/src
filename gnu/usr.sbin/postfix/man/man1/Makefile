#	$NetBSD: Makefile,v 1.2 2001/03/13 18:13:19 itojun Exp $

DIST=	${.CURDIR}/../../../../dist/postfix/man/man1
.PATH:	${DIST}

MAN=	postalias.1 postcat.1 postconf.1 postfix.1 \
	postkick.1 postlock.1 postlog.1 postdrop.1 \
	postmap.1 \
	postsuper.1

# XXX notyet:
# mailq.1 sendmail.1 newaliases.1
# These would conflict with the ones installed by sendmail. It isn't
# clear what to do with them. The missing documentation is not too
# interesting given that the sendmail docs are in place, though.

.include <bsd.man.mk>
