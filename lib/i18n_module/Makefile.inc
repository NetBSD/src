#	$NetBSD: Makefile.inc,v 1.4 2003/03/09 00:54:44 lukem Exp $

NOLINT=		# defined
NOPICINSTALL=	# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

SHLIB_VERSION_FILE=	${NETBSDSRCDIR}/lib/i18n_module/shlib_version
LIBDIR=		/usr/lib/i18n
SHLIBDIR=	/usr/lib/i18n
SHLIBINSTALLDIR=/usr/lib/i18n
INTERNALLIB=	yes
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/citrus
CPPFLAGS+=	-DLOCALEMOD_MAJOR=${SHLIB_MAJOR}
CPPFLAGS+=	-DLIBC_SCCS
BASENAME!=	basename ${.CURDIR}
LIB=		${BASENAME}
SRCS!=		echo ${SRCPRE}${BASENAME}.c | tr A-Z a-z

.PATH:	${NETBSDSRCDIR}/lib/libc/citrus/modules
