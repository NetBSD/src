#	$NetBSD: Makefile,v 1.2 2003/07/29 07:26:23 mrg Exp $

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

.include <bsd.lib.mk>

${OBJS} .depend: multilib.h specs.h

.PATH: ${DIST}/gcc

#
# Build the specs.h file
#
CLEANFILES+=	specs.h
specs.h: Makefile
	for f in cp f objc; do \
		echo "#include \"$$f/lang-specs.h\""; \
	done > specs.h
