#	$NetBSD: Makefile,v 1.6 1997/11/20 00:12:33 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	canfield
MAN=	canfield.6
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermcap
HIDEGAME=hidegame
SETGIDGAME=yes


.include "../../Makefile.inc"
.include <bsd.prog.mk>
