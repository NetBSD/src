#	$NetBSD: Makefile,v 1.4 1997/04/17 22:17:27 thorpej Exp $

PROG=	ld.elf_so

# Adds SRCS, CFLAGS, LDFLAGS, etc.  Must go first so MD startup source
# is first.
.if exists(${.CURDIR}/${MACHINE_ARCH}/Makefile.inc)
.include "${.CURDIR}/${MACHINE_ARCH}/Makefile.inc"
.endif

SRCS+=	rtld.c reloc.c symbol.c malloc.c xmalloc.c xprintf.c debug.c \
	map_object.c load.c search.c headers.c paths.c
CFLAGS+= -Wall -DLIBDIR=\"${LIBDIR}\" -D_PATH_RTLD=\"${BINDIR}/${PROG}\"
CFLAGS+= -DDEBUG -DRTLD_LOADER
#CFLAGS+= -DRTLD_DEBUG

LDADD+=	-L${LIBDIR} -non_shared -lc_pic
DPADD+=	${LIBC_PIC}

# to be installed
HDRS=	link.h

NOMAN=
STRIPFLAG=

.PATH: ${.CURDIR}/${MACHINE_ARCH}

${PROG}: ${OBJS} ${DPADD}
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LDADD}

includes:
	@cd ${.CURDIR}; for i in $(HDRS); do \
	    j="cmp -s $$i ${DESTDIR}/usr/include/$$i || \
	    ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}/usr/include"; \
	    echo $$j; \
	    eval "$$j"; \
	done

.include <bsd.prog.mk>
