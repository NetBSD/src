#	$NetBSD: Makefile.inc,v 1.5 2001/12/27 11:54:02 lukem Exp $

NOLINT=		# defined
NOPICINSTALL=	# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

.include "../Makefile.inc"

SHLIB_VERSION_FILE=	${.CURDIR}/../shlib_version
LIBDIR=		/usr/lib/runemodule
INTERNALLIB=	yes
CPPFLAGS+=	-I${.CURDIR}/../../libc/locale
CPPFLAGS+=	-DRUNEMOD_MAJOR=${SHLIB_MAJOR}
BASENAME!=	basename ${.CURDIR}
LIB=		${BASENAME}
SRCS!=		echo ${BASENAME}.c | tr A-Z a-z

.PATH:	${.CURDIR}/../../libc/locale
