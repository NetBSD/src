#	$NetBSD: Makefile,v 1.5 1997/05/26 03:57:44 cjs Exp $

.include <bsd.own.mk>	# for BUILDDIR

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

.if defined(BUILDDIR)
includes:
	@cd ${.CURDIR}; for i in $(HDRS); do \
	    j="cmp -s $$i ${BUILDDIR}/usr/include/$$i || \
	    install -d ${BUILDDIR}/usr/include && \
	    ${INSTALL} -c -m 444 $$i ${BUILDDIR}/usr/include"; \
	    echo $$j; \
	    eval "$$j"; \
	done
.else
includes: beforeinstall
.endif

beforeinstall:
	@cd ${.CURDIR}; for i in $(HDRS); do \
	    j="cmp -s $$i ${DESTDIR}/usr/include/$$i || \
	    ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}/usr/include"; \
	    echo $$j; \
	    eval "$$j"; \
	done

.include <bsd.prog.mk>
