#	$NetBSD: Makefile,v 1.3 2000/05/31 17:02:55 perry Exp $

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${.CURDIR}/../../../dist/postfix/conf
.PATH:	${DIST}

# Not built yet: main.cf.default

FILES=		LICENSE access aliases canonical main.cf master.cf \
		relocated sample-aliases.cf sample-canonical.cf \
		sample-debug.cf sample-filter.cf sample-ldap.cf \
		sample-local.cf \
		sample-misc.cf sample-pcre.cf sample-rate.cf \
		sample-regexp.cf sample-relocated.cf \
		sample-resource.cf sample-rewrite.cf \
		sample-smtp.cf sample-smtpd.cf sample-transport.cf \
		sample-virtual.cf transport virtual

SCRIPTS=	postfix-script-sgid
SCRIPTSNAME=	postfix-script

# We don't install a few things that are found in this directory:
# postfix-script: the source version here is just a placeholder.
# postfix-script-diff: uninteresting to the user.
# postfix-script-nosgid: we don't use that version.

.include <bsd.prog.mk>

