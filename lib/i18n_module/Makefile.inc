#	$NetBSD: Makefile.inc,v 1.6 2003/12/13 04:28:50 matt Exp $

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
BASENAME=	${.CURDIR:T}
LIB=		${BASENAME}
XNAME=		${SRCPRE}${BASENAME}.c 
SRCS?=		${XNAME:tl}

.PATH:	${NETBSDSRCDIR}/lib/libc/citrus/modules
