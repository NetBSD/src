#	$NetBSD: Makefile,v 1.25 2001/12/28 01:32:39 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

LIB=	termcap
USE_SHLIBDIR=	yes
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
LINKS+=	${_LIBSODIR}/libtermcap.so.${SHLIB_FULLVERSION} \
	${_LIBSODIR}/libtermlib.so.${SHLIB_FULLVERSION}
.if ${_LIBSODIR} != ${LIBDIR}
SYMLINKS+=${_LIBSODIR}/libtermcap.so.${SHLIB_FULLVERSION} \
	${LIBDIR}/libtermlib.so.${SHLIB_FULLVERSION}
.endif

.if (${OBJECT_FMT} == "ELF")
LINKS+=	${_LIBSODIR}/libtermcap.so.${SHLIB_MAJOR} \
	${_LIBSODIR}/libtermlib.so.${SHLIB_MAJOR}
LINKS+=	${_LIBSODIR}/libtermcap.so ${_LIBSODIR}/libtermlib.so
.if ${_LIBSODIR} != ${LIBDIR}
SYMLINKS+=	${_LIBSODIR}/libtermcap.so.${SHLIB_MAJOR} \
		${LIBDIR}/libtermlib.so.${SHLIB_MAJOR}
SYMLINKS+=	${_LIBSODIR}/libtermcap.so ${LIBDIR}/libtermlib.so
.endif
.endif
.endif	# exists shlib_version

.endif	# ${MKPIC} != "no"

.include <bsd.lib.mk>
