#	$NetBSD: bsd.gcc.mk,v 1.1 2003/05/31 20:33:15 thorpej Exp $

.if !defined(_BSD_GCC_MK_)
_BSD_GCC_MK_=1

.if defined(EXTERNAL_TOOLCHAIN)
_GCC_CRTBEGIN!=		${CC} --print-file-name=crtbegin.o
_GCC_CRTBEGINS!=	${CC} --print-file-name=crtbeginS.o
_GCC_CRTEND!=		${CC} --print-file-name=crtend.o
_GCC_CRTENDS!=		${CC} --print-file-name=crtendS.o
_GCC_LIBGCCDIR!=	dirname `${CC} --print-libgcc-file-name`
.else
_GCC_CRTBEGIN=		${DESTDIR}/usr/lib/crtbegin.o
_GCC_CRTBEGINS=		${DESTDIR}/usr/lib/crtbeginS.o
_GCC_CRTEND=		${DESTDIR}/usr/lib/crtend.o
_GCC_CRTENDS=		${DESTDIR}/usr/lib/crtendS.o
_GCC_LIBGCCDIR=		${DESTDIR}/usr/lib
.endif

.endif	# ! defined(_BSD_GCC_MK_)
