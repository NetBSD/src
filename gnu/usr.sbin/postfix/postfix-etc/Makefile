#	$NetBSD: Makefile,v 1.7 2004/05/16 09:53:10 lukem Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/conf
.PATH:	${DIST}

FILESDIR=	${PFIX_ETCDIR}

# These two .cf files are the only ones absolutely needed. It is not
# clear if we should be installing things like the transport and
# virtual file, etc. I contend we shouldn't, since they contain no
# real information at all, just the man page sources, and are not
# needed by default.
#
CONFIGFILES=	main.cf master.cf postfix-files README

# And of course, we need the postfix-script file. It isn't clear that
# this should be considered configuration and not a candidate for
# libexec, but Wietse explicitly wants it this way, since he wants
# the user to be able to use postfix-script as a place to install hooks.
#
CONFIGFILES+=			postfix-script post-install
FILESMODE_postfix-script=	${BINMODE}
FILESMODE_post-install=		${BINMODE}

.include <bsd.prog.mk>
