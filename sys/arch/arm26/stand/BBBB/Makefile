#	$NetBSD: Makefile,v 1.1 2000/11/21 23:47:38 bjh21 Exp $

#MAN=	BBBB.8

.if !defined(BASTOK)
.ifnmake(print-objdir)
BASTOKDIR!=	cd ${.CURDIR}/../bastok; ${MAKE} print-objdir
BASTOK=		${BASTOKDIR}/bastok
BASTOKTARG=	${BASTOK}

${BASTOK}:
	(cd ${.CURDIR}/../bastok; ${MAKE})

.endif
.else
BASTOKTARG=
.endif

clean cleandir distclean: bbbbclean
realall: BBBB,ffb

bbbbclean:
	rm -f BBBB,ffb

BBBB,ffb: BBBB,fd1 ${BASTOKTARG}
	${BASTOK} ${.CURDIR}/BBBB,fd1 > BBBB,ffb

.include <bsd.man.mk>
