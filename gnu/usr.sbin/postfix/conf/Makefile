#	$NetBSD: Makefile,v 1.4 2001/03/13 18:13:17 itojun Exp $

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${.CURDIR}/../../../dist/postfix/conf
.PATH:	${DIST}

# Not built yet: main.cf.default

FILES=		LICENSE access aliases canonical main.cf master.cf \
		pcre_table regexp_table relocated sample-aliases.cf \
		sample-auth.cf sample-canonical.cf sample-compatibility.cf \
		sample-debug.cf sample-filter.cf sample-flush.cf \
		sample-ldap.cf sample-lmtp.cf sample-local.cf \
		sample-misc.cf sample-pcre.cf sample-rate.cf \
		sample-regexp.cf sample-relocated.cf sample-resource.cf \
		sample-rewrite.cf sample-smtp.cf sample-smtpd.cf \
		sample-transport.cf sample-virtual.cf transport \
		virtual

SCRIPTS=	postfix-script-sgid
SCRIPTSNAME=	postfix-script

# We don't install a few things that are found in this directory:
# postfix-script: the source version here is just a placeholder.
# postfix-script-diff: uninteresting to the user.
# postfix-script-nosgid: we don't use that version.

.include <bsd.prog.mk>
