#	$NetBSD: Makefile,v 1.3 2002/02/02 23:21:47 perry Exp $

DIST=	${.CURDIR}/../../../../dist/postfix/man/man5
.PATH:	${DIST}

MAN=	access.5 canonical.5 pcre_table.5 regexp_table.5 \
	relocated.5 transport.5 virtual.5

# XXX notyet:
# aliases.5
# This would conflict with the one installed by sendmail. It isn't
# clear what to do with it. The missing documentation is not too
# interesting given that the sendmail docs are in place, though. The
# format is identical.

.include <bsd.man.mk>
