#	from: @(#)Makefile	5.1 (Berkeley) 6/18/90
#	$Id: Makefile,v 1.3 1993/08/01 05:42:00 mycroft Exp $

LIB=	ln
SRCS=	libmain.c
LINKS=	${LIBDIR}/libln.a ${LIBDIR}/libl.a
.if !defined(NOPROFILE)
LINKS+=	${LIBDIR}/libln_p.a ${LIBDIR}/libl_p.a
.endif

.include <bsd.lib.mk>
