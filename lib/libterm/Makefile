#	$NetBSD: Makefile,v 1.11 1997/10/23 03:27:34 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

LIB=	termcap
CPPFLAGS+=-DCM_N -DCM_GT -DCM_B -DCM_D
SRCS=	termcap.c tgoto.c tputs.c

MAN=	termcap.3
MLINKS=	termcap.3 tgetent.3 termcap.3 tgetflag.3 termcap.3 tgetnum.3 \
	termcap.3 tgetstr.3 termcap.3 tgoto.3 termcap.3 tputs.3
LINKS=	${LIBDIR}/libtermcap.a ${LIBDIR}/libtermlib.a
INCS=	termcap.h
INCSDIR=/usr/include

.include <bsd.lib.mk>

.if !defined(NOPROFILE)
LINKS+=	${LIBDIR}/libtermcap_p.a ${LIBDIR}/libtermlib_p.a
.endif

.if !defined(NOPIC) && defined(SHLIB_MAJOR) && defined(SHLIB_MINOR)
LINKS+=	${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	${LIBDIR}/libtermlib.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
.endif
