#	$NetBSD: Makefile,v 1.13 2003/03/25 22:48:43 mycroft Exp $

LIB=	z
NOPIC=	# defined
NOPROFILE=# defined

CPPFLAGS=	-I${ZDIR} ${ZCPPFLAGS} ${ZMISCCPPFLAGS} -D_ZLIB_PRIVATE

.PATH.c: ${ZDIR}

# files to be copied down from libz.
LIBZSRCS= crc32.c infblock.c infcodes.c inffast.c \
        inflate.c inftrees.c infutil.c uncompr.c
LIBZHDRS= infblock.h infcodes.h inffast.h inftrees.h infutil.h \
	zconf.h zlib.h zutil.h

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

update-sources:
	-@for file in ${LIBZSRCS} ${LIBZHDRS}; do \
		tail +2 ${.CURDIR}/../../../lib/libz/$$file > .tmp1.$$file ; \
		tail +2 ${.CURDIR}/$$file > .tmp2.$$file ; \
		cmp -s .tmp1.$$file .tmp2.$$file || ( echo Updating $$file ; \
		  cp ${.CURDIR}/../../../lib/libz/$$file ${.CURDIR} ) ; \
		rm .tmp1.$$file .tmp2.$$file ; \
	done
