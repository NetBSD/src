#	$NetBSD: Makefile,v 1.19.4.1 2015/09/22 12:06:08 skrll Exp $

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

lib${LIB}.o:: ${OBJS:O}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`
	@echo done

.if defined(HAVE_GCC) && ${MACHINE_ARCH} == "vax"
COPTS.inftrees.c+=	-O0
.endif
