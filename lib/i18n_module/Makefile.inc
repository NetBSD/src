#	$NetBSD: Makefile.inc,v 1.5 2003/06/25 10:38:03 tshiozak Exp $

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
.ifndef SRCS
SRCS!=		echo ${SRCPRE}${BASENAME}.c | tr A-Z a-z
.endif

.PATH:	${NETBSDSRCDIR}/lib/libc/citrus/modules
