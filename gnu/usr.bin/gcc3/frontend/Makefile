#	$NetBSD: Makefile,v 1.4 2003/08/01 17:03:46 lukem Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=		frontend

SRCS=		gcc.c version.c intl.c prefix.c

CPPFLAGS+=	-I${.CURDIR} -I${GCCARCH} -I${BACKENDOBJ} -I. \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\" \
		-DDEFAULT_TARGET_VERSION=\"${G_version}\" \
		-DDEFAULT_TARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\"
HOST_CPPFLAGS+=	-I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

DPSRCS+=	multilib.h specs.h

.include <bsd.lib.mk>

.PATH: ${DIST}/gcc

#
# Build the specs.h file
#
CLEANFILES+=	specs.h
specs.h: Makefile
	for f in cp f objc; do \
		echo "#include \"$$f/lang-specs.h\""; \
	done > specs.h
