#	$NetBSD: Makefile,v 1.27 2002/08/19 14:55:17 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

USE_SHLIBDIR=	yes

LIB=	termcap
CPPFLAGS+=-I${.CURDIR} -DCM_N -DCM_GT -DCM_B -DCM_D
SRCS=	termcap.c tgoto.c tputs.c

MAN=	termcap.3
MLINKS=	termcap.3 tgetent.3 termcap.3 tgetflag.3 termcap.3 tgetnum.3 \
	termcap.3 tgetstr.3 termcap.3 tgoto.3 termcap.3 tputs.3 \
	termcap.3 t_getent.3 termcap.3 t_getflag.3 termcap.3 t_getnum.3 \
	termcap.3 t_getstr.3 termcap.3 t_goto.3 termcap.3 t_puts.3 \
	termcap.3 t_freent.3
INCS=	termcap.h
INCSDIR=/usr/include

.include <bsd.own.mk>

.if ${MKLINKLIB} != "no"
SYMLINKS+=	libtermcap.a ${LIBDIR}/libtermlib.a
.endif

.if ${MKPROFILE} != "no"
SYMLINKS+=	libtermcap_p.a ${LIBDIR}/libtermlib_p.a
.endif

.if ${MKPIC} != "no"

.if ${MKPICINSTALL} != "no"
SYMLINKS+=	libtermcap_pic.a ${LIBDIR}/libtermlib_pic.a
.endif

.if exists(${.CURDIR}/shlib_version)
SYMLINKS+=	libtermcap.so.${SHLIB_FULLVERSION} \
		${_LIBSODIR}/libtermlib.so.${SHLIB_FULLVERSION}
.if ${_LIBSODIR} != ${LIBDIR}
SYMLINKS+=	libtermcap.so.${SHLIB_FULLVERSION} \
		${LIBDIR}/libtermlib.so.${SHLIB_FULLVERSION}
.endif

.if (${OBJECT_FMT} == "ELF")
SYMLINKS+=	libtermcap.so.${SHLIB_MAJOR} \
		${_LIBSODIR}/libtermlib.so.${SHLIB_MAJOR}
SYMLINKS+=	libtermcap.so ${_LIBSODIR}/libtermlib.so
.if ${_LIBSODIR} != ${LIBDIR}
SYMLINKS+=	libtermcap.so.${SHLIB_MAJOR} \
		${LIBDIR}/libtermlib.so.${SHLIB_MAJOR}
SYMLINKS+=	libtermcap.so ${LIBDIR}/libtermlib.so
.endif
.endif
.endif	# exists shlib_version

.endif	# ${MKPIC} != "no"

.include <bsd.lib.mk>
