#	$NetBSD: Makefile.inc,v 1.1 2002/03/17 22:14:13 tshiozak Exp $

NOLINT=		# defined
NOPICINSTALL=	# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

SHLIB_VERSION_FILE=	${.CURDIR}/../shlib_version
LIBDIR=		/usr/lib/i18n
SHLIBDIR=	/usr/lib/i18n
INTERNALLIB=	yes
CPPFLAGS+=	-I${.CURDIR}/../../libc/citrus
CPPFLAGS+=	-DLOCALEMOD_MAJOR=${SHLIB_MAJOR}
BASENAME!=	basename ${.CURDIR}
LIB=		${BASENAME}
SRCS!=		echo ${SRCPRE}${BASENAME}.c | tr A-Z a-z

.PATH:	${.CURDIR}/../../libc/citrus/modules
