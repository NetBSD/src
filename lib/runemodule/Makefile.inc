#	$NetBSD: Makefile.inc,v 1.3 2001/11/05 15:04:54 lukem Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"

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
