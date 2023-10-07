#	$NetBSD: Makefile,v 1.24 2023/10/07 12:15:54 rin Exp $

LIB=	z
LIBISPRIVATE=	yes

ZDISTDIR:=${.PARSEDIR}/../../../common/dist/zlib

CPPFLAGS+=	-I${ZDISTDIR} ${ZCPPFLAGS} ${ZMISCCPPFLAGS}

.PATH.c: ${ZDISTDIR} ${.PARSEDIR}

# files to be copied down from libz.
LIBZSRCS= adler32.c compress.c deflate.c infback.c inffast.c \
	inflate.c inftrees.c trees.c uncompr.c
LIBZHDRS= deflate.h inffast.h inffixed.h inflate.h inftrees.h \
	trees.h zconf.h zlib.h

# Other stuff
SRCS=	${LIBZSRCS} zalloc.c

# Files to clean up
CLEANFILES+= lib${LIB}.o

.undef DESTDIR
.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS:O} __buildstdlib
