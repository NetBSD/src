#	$NetBSD: Makefile,v 1.5 2002/02/02 23:21:46 perry Exp $

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${.CURDIR}/../../../dist/postfix/conf
.PATH:	${DIST}

# Not built: main.cf.default

FILES=	LICENSE access aliases canonical main.cf master.cf \
	pcre_table post-install postfix-files \
	regexp_table relocated sample-aliases.cf sample-auth.cf \
	sample-canonical.cf sample-compatibility.cf sample-debug.cf \
	sample-filter.cf sample-flush.cf sample-ldap.cf \
	sample-lmtp.cf sample-local.cf sample-misc.cf \
	sample-pcre-access.cf sample-pcre-body.cf sample-pcre-header.cf \
	sample-qmqpd.cf sample-rate.cf sample-regexp-access.cf \
	sample-regexp-body.cf sample-regexp-header.cf \
	sample-relocated.cf sample-resource.cf sample-rewrite.cf \
	sample-smtp.cf sample-smtpd.cf sample-transport.cf \
	sample-virtual.cf transport virtual

SCRIPTS=	postfix-script

.include <bsd.prog.mk>
