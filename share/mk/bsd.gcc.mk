#	$NetBSD: bsd.gcc.mk,v 1.8 2012/12/10 20:58:55 pooka Exp $

.if !defined(_BSD_GCC_MK_)
_BSD_GCC_MK_=1

.if defined(EXTERNAL_TOOLCHAIN)
_GCC_CRTBEGIN!=		${CC} --print-file-name=crtbegin.o
_GCC_CRTEND!=		${CC} --print-file-name=crtend.o

# If the toolchain does not know about a file, --print-file-name echoes
# the input file name (why??).  In that case we want an empty string for
# dependencies to work correctly.  Note: do _not_ use TOOL_SED here because
# this file is used before TOOL_SED is built.
_GCC_CRTBEGINS!=	${CC} --print-file-name=crtbeginS.o		\
			    | sed 's|^crtbeginS.o$$||'
_GCC_CRTENDS!=		${CC} --print-file-name=crtendS.o		\
			    | sed 's|^crtendS.o$$||'
_GCC_CRTI!=		${CC} --print-file-name=crti.o			\
			    | sed 's|^crti.o$$||'
_GCC_CRTN!=		${CC} --print-file-name=crtn.o			\
			    | sed 's|^crtn.o$$||'

_GCC_CRTDIR!=    	dirname ${_GCC_CRTBEGIN}
_GCC_LIBGCCDIR!= 	dirname `${CC} --print-libgcc-file-name`

.else

_GCC_CRTBEGIN?=		${DESTDIR}/usr/lib/crtbegin.o
_GCC_CRTBEGINS?=	${DESTDIR}/usr/lib/crtbeginS.o
_GCC_CRTEND?=		${DESTDIR}/usr/lib/crtend.o
_GCC_CRTENDS?=		${DESTDIR}/usr/lib/crtendS.o
_GCC_CRTI?=		${DESTDIR}/usr/lib/crti.o
_GCC_CRTN?=		${DESTDIR}/usr/lib/crtn.o
_GCC_CRTDIR?=		${DESTDIR}/usr/lib
_GCC_LIBGCCDIR?=	${DESTDIR}/usr/lib
.endif

.endif	# ! defined(_BSD_GCC_MK_)
