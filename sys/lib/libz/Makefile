#	$NetBSD: Makefile,v 1.8 1999/04/15 00:52:58 simonb Exp $

LIB=	z
MKPIC=	no
MKPROFILE=no

CPPFLAGS=	-I. ${ZCPPFLAGS} ${ZMISCCPPFLAGS} -D_ZLIB_PRIVATE

# files to be copied down from libz.
LIBZSRCS= adler32.c crc32.c infblock.c infcodes.c inffast.c \
        inflate.c inftrees.c infutil.c uncompr.c
LIBZHDRS= infblock.h infcodes.h inffast.h inftrees.h infutil.h \
	zconf.h zlib.h zutil.h

# Other stuff
SRCS=	${LIBZSRCS} zalloc.c

# Files to clean up
CLEANFILES+= lib${LIB}.o

# only needed during build
libinstall::

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`

update-sources:
	for file in ${LIBZSRCS} ${LIBZHDRS}; do \
		cp ${.CURDIR}/../../../lib/libz/$$file ${.CURDIR}; \
	done
