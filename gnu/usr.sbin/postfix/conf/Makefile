#	$NetBSD: Makefile,v 1.1 2000/04/30 21:21:55 perry Exp $

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${.CURDIR}/../../../dist/postfix/conf
.PATH:	${DIST}

# Not built yet: main.cf.default

FILES=	LICENSE access aliases canonical main.cf master.cf \
	relocated sample-aliases.cf sample-canonical.cf \
	sample-debug.cf sample-ldap.cf sample-local.cf \
	sample-misc.cf sample-pcre.cf sample-rate.cf \
	sample-regexp.cf sample-relocated.cf \
	sample-resource.cf sample-rewrite.cf \
	sample-smtp.cf sample-smtpd.cf sample-transport.cf \
	sample-virtual.cf transport virtual

# We don't install a few things that are found in this directory:
# postfix-script: installed (from postfix-script-sgid) in
# ../postfix-script -- the version here is just a placeholder anyway.
# postfix-script-diff: uninteresting to the user.
# postfix-script-nosgid postfix-script-sgid: see above.

.include <bsd.prog.mk>

