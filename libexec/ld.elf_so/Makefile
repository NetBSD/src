#	$NetBSD: Makefile,v 1.18 1999/01/10 18:18:56 christos Exp $

.include <bsd.own.mk>			# for OBJECT_FMT definition

.if ((${OBJECT_FMT} == "ELF") && !defined(NOPIC))
PROG=	ld.elf_so
.endif

.if (${MACHINE} == "alpha") || (${MACHINE_ARCH} == "mips") || \
    (${MACHINE_ARCH} == "powerpc") || (${MACHINE} == "sparc64") || \
    (${MACHINE} == "i386")
# Adds SRCS, CPPFLAGS, LDFLAGS, etc.  Must go first so MD startup source
# is first.
.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc)
.include "${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc"
.endif

CLIBOBJ!=cd ${.CURDIR}/../../lib/libc; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

SRCS+=	rtld.c reloc.c symbol.c malloc.c xmalloc.c xprintf.c debug.c \
	map_object.c load.c search.c headers.c paths.c
CPPFLAGS+= -Wall -DLIBDIR=\"${LIBDIR}\" -D_PATH_RTLD=\"${BINDIR}/${PROG}\"
CPPFLAGS+= -DDEBUG -DRTLD_LOADER
#CPPFLAGS+= -DRTLD_DEBUG

LDADD+=	-L${DESTDIR}/${LIBDIR} -L${CLIBOBJ} -non_shared -lc_pic
DPADD+=	${LIBC_PIC}

.if (${OBJECT_FMT} == "ELF")
# to be installed
HDRS=	link.h
.endif

STRIPFLAG=

.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}

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
