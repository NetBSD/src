#	$NetBSD: Makefile,v 1.9 2006/05/30 19:32:28 tron Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/man/man1
.PATH:	${DIST}

MAN=	mailq.1 newaliases.1 \
	postalias.1 postcat.1 postconf.1 postfix.1 \
	postkick.1 postlock.1 postlog.1 postdrop.1 \
	postmap.1 \
	postqueue.1 postsuper.1 \
	sendmail.1

# XXX notyet:
# qmqp-sink.1 qmqp-source.1 smtp-sink.1 smtp-source.1 qshape.1
# These are test programs. It isn't clear that they are wanted by an
# ordinary user.

.include <bsd.man.mk>
