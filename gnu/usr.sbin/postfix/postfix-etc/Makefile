#	$NetBSD: Makefile,v 1.5 2002/02/03 00:50:36 perry Exp $

# This Makefile is called only by src/etc, not by usr.sbin/postfix/Makefile

DIST=	${.CURDIR}/../../../dist/postfix/conf
.PATH:	${DIST}

BINDIR=	${PFIX_ETCDIR}

# These two .cf files are the only ones absolutely needed. It is not
# clear if we should be installing things like the transport and
# virtual file, etc. I contend we shouldn't, since they contain no
# real information at all, just the man page sources, and are not
# needed by default.

FILES=	main.cf master.cf postfix-files README

# And of course, we need the postfix-script file. It isn't clear that
# this should be considered configuration and not a candidate for
# libexec, but Wietse explicitly wants it this way, since he wants
# the user to be able to use postfix-script as a place to install hooks.
SCRIPTS=	postfix-script post-install

.include <bsd.prog.mk>
