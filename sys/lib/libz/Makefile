#	$NetBSD: Makefile,v 1.14 2006/01/14 20:15:38 christos Exp $

LIB=	z
NOPIC=	# defined
NOPROFILE=# defined

ZDISTDIR:=${.PARSEDIR}/../../../common/dist/zlib

CPPFLAGS+=	-I${ZDISTDIR} ${ZCPPFLAGS} ${ZMISCCPPFLAGS} -D_ZLIB_PRIVATE

.PATH.c: ${ZDISTDIR} ${.PARSEDIR}

# files to be copied down from libz.
LIBZSRCS= adler32.c compress.c crc32.c deflate.c infback.c inffast.c \
	inflate.c inftrees.c trees.c uncompr.c
LIBZHDRS= crc32.h deflate.h inffast.h inffixed.h inflate.h inftrees.h \
	trees.h zconf.h zlib.h

# Other stuff
SRCS=	${LIBZSRCS} zalloc.c

# Files to clean up
CLEANFILES+= lib${LIB}.o

# only needed during build
libinstall::

.undef DESTDIR
.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`
