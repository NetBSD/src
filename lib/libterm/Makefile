#	$NetBSD: Makefile,v 1.19 1999/02/12 11:32:32 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

LIB=	termcap
CPPFLAGS+=-DCM_N -DCM_GT -DCM_B -DCM_D
SRCS=	termcap.c tgoto.c tputs.c

MAN=	termcap.3
MLINKS=	termcap.3 tgetent.3 termcap.3 tgetflag.3 termcap.3 tgetnum.3 \
	termcap.3 tgetstr.3 termcap.3 tgoto.3 termcap.3 tputs.3
INCS=	termcap.h
INCSDIR=/usr/include

.include <bsd.own.mk>

.if !defined(NOSTATICLIB)
LINKS+=	${LIBDIR}/libtermcap.a ${LIBDIR}/libtermlib.a
.endif

.if ${MKPROFILE} != "no"
LINKS+=	${LIBDIR}/libtermcap_p.a ${LIBDIR}/libtermlib_p.a
.endif

.if ${MKPIC} != "no"

.if ${MKPICINSTALL} != "no"
LINKS+=	${LIBDIR}/libtermcap_pic.a ${LIBDIR}/libtermlib_pic.a
.endif

.if exists(${.CURDIR}/shlib_version)
LINKS+=	${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	${LIBDIR}/libtermlib.so.${SHLIB_MAJOR}.${SHLIB_MINOR}

.if (${OBJECT_FMT} == "ELF")
LINKS+=	${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR} \
	${LIBDIR}/libtermlib.so.${SHLIB_MAJOR}
LINKS+=	${LIBDIR}/lib${LIB}.so ${LIBDIR}/libtermlib.so
.endif
.endif

.endif	# ${MKPIC} != "no"

.include <bsd.lib.mk>
