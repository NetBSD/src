#	$NetBSD: Makefile,v 1.7.2.4 1998/05/08 17:39:12 mycroft Exp $

.if (${MACHINE} == "alpha") || (${MACHINE_ARCH} == "mips") 
PROG=	ld.elf_so

# Adds SRCS, CPPFLAGS, LDFLAGS, etc.  Must go first so MD startup source
# is first.
.if exists(${.CURDIR}/${MACHINE_ARCH}/Makefile.inc)
.include "${.CURDIR}/${MACHINE_ARCH}/Makefile.inc"
.endif

SRCS+=	rtld.c reloc.c symbol.c malloc.c xmalloc.c xprintf.c debug.c \
	map_object.c load.c search.c headers.c paths.c
CPPFLAGS+= -Wall -DLIBDIR=\"${LIBDIR}\" -D_PATH_RTLD=\"${BINDIR}/${PROG}\"
CPPFLAGS+= -DDEBUG -DRTLD_LOADER
#CPPFLAGS+= -DRTLD_DEBUG

LDADD+=	-L${DESTDIR}/${LIBDIR} -non_shared -lc_pic
DPADD+=	${LIBC_PIC}

# to be installed
HDRS=	link.h

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
.endif

NOMAN=

.include <bsd.prog.mk>
