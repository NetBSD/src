#	$NetBSD: bsd.gcc.mk,v 1.11.14.1 2018/05/21 04:35:57 pgoyette Exp $

.if !defined(_BSD_GCC_MK_)
_BSD_GCC_MK_=1

_GCC_CRTBEGIN?=		${DESTDIR}/usr/lib/${MLIBDIR:D${MLIBDIR}/}crtbegin.o
_GCC_CRTBEGINS?=	${DESTDIR}/usr/lib/${MLIBDIR:D${MLIBDIR}/}crtbeginS.o
_GCC_CRTEND?=		${DESTDIR}/usr/lib/${MLIBDIR:D${MLIBDIR}/}crtend.o
_GCC_CRTENDS?=		${DESTDIR}/usr/lib/${MLIBDIR:D${MLIBDIR}/}crtendS.o
_GCC_CRTI?=		${DESTDIR}/usr/lib/${MLIBDIR:D${MLIBDIR}/}crti.o
_GCC_CRTN?=		${DESTDIR}/usr/lib/${MLIBDIR:D${MLIBDIR}/}crtn.o
_GCC_CRTDIR?=		${DESTDIR}/usr/lib${MLIBDIR:D/${MLIBDIR}}
_GCC_LIBGCCDIR?=	${DESTDIR}/usr/lib${MLIBDIR:D/${MLIBDIR}}

.endif	# ! defined(_BSD_GCC_MK_)
