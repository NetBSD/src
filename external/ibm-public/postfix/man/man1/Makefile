#	$NetBSD: Makefile,v 1.1.26.1 2014/08/10 07:12:51 tls Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/man/man1
.PATH:	${DIST}

# This follows the order in COMMANDS= in src/external/ibm-public/postfix/dist/man/Makefile.in
MAN=	postalias.1 postcat.1 postconf.1 postfix.1 \
	postkick.1 postlock.1 postlog.1 postdrop.1 \
	postmap.1 sendmail.1 \
	postqueue.1 postsuper.1 postmulti.1 posttls-finger.1

MLINKS=	sendmail.1 mailq.1 sendmail.1 newaliases.1

# XXX notyet:
# qmqp-sink.1 qmqp-source.1 smtp-sink.1 smtp-source.1 qshape.1
# These are test programs. It isn't clear that they are wanted by an
# ordinary user.

.include <bsd.man.mk>
