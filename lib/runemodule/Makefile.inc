#	$NetBSD: Makefile.inc,v 1.1 2001/01/25 09:16:39 itojun Exp $

.include <bsd.own.mk>

WARNS?=	1

SHLIB_VERSION_FILE=	${.CURDIR}/../shlib_version
SHLIBDIR=	/usr/lib/runemodule
LIBDIR=		/usr/lib/runemodule
INTERNALLIB=	yes
MKPROFILE=	no
MKPICINSTALL=	no
MKLINT=		no
CPPFLAGS+=	-I${.CURDIR}/../../libc/locale
CPPFLAGS+=	-DRUNEMOD_MAJOR=${SHLIB_MAJOR}
BASENAME!=	basename ${.CURDIR}
LIB=		${BASENAME}
SRCS!=		echo ${BASENAME}.c | tr A-Z a-z

.PATH:	${.CURDIR}/../../libc/locale

beforeinstall:
	if [ ! -d ${SHLIBDIR} ]; then mkdir ${SHLIBDIR}; fi
