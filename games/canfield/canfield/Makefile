#	$NetBSD: Makefile,v 1.5.4.1 1997/11/20 09:06:32 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	canfield
MAN=	canfield.6
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermcap
HIDEGAME=hidegame
SETGIDGAME=yes


.include "../../Makefile.inc"
.include <bsd.prog.mk>
