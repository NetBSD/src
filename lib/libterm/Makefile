#	from: @(#)Makefile	5.10 (Berkeley) 6/1/90
#	$Id: Makefile,v 1.6 1994/12/22 09:57:51 cgd Exp $

LIB=	termcap
CFLAGS+=-DCM_N -DCM_GT -DCM_B -DCM_D
SRCS=	termcap.c tgoto.c tputs.c

MAN=	termcap.3
MLINKS=	termcap.3 tgetent.3 termcap.3 tgetflag.3 termcap.3 tgetnum.3 \
	termcap.3 tgetstr.3 termcap.3 tgoto.3 termcap.3 tputs.3
LINKS=	${LIBDIR}/libtermcap.a ${LIBDIR}/libtermlib.a

.include <bsd.lib.mk>

.if !defined(NOPROFILE)
LINKS+=	${LIBDIR}/libtermcap_p.a ${LIBDIR}/libtermlib_p.a
.endif

.if !defined(NOPIC) && defined(SHLIB_MAJOR) && defined(SHLIB_MINOR)
LINKS+=	${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	${LIBDIR}/libtermlib.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
.endif
