#	$NetBSD: Makefile,v 1.23 1999/03/03 12:00:19 jonathan Exp $

.include <bsd.own.mk>			# for OBJECT_FMT definition

.if ${OBJECT_FMT} == "ELF" && ${MKPIC} != "no"
PROG=	ld.elf_so
.endif

M=	${.CURDIR}/arch/${MACHINE_ARCH:S/mipse[bl]/mips/}

.if (${MACHINE} == "alpha") || \
    (${MACHINE_ARCH} == "mipsel") || (${MACHINE_ARCH} == "mipseb") || \
    (${MACHINE_ARCH} == "powerpc") || (${MACHINE} == "sparc64") || \
    (${MACHINE} == "i386") || (${MACHINE} == "sparc")
# Adds SRCS, CPPFLAGS, LDFLAGS, etc.  Must go first so MD startup source
# is first.
.if exists($M/Makefile.inc)
.include "$M/Makefile.inc"
.endif

CLIBOBJ!=cd ${.CURDIR}/../../lib/libc; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

SRCS+=	rtld.c reloc.c symbol.c malloc.c xmalloc.c xprintf.c debug.c \
	map_object.c load.c search.c headers.c paths.c
CPPFLAGS+= -Wall -DLIBDIR=\"${LIBDIR}\" -D_PATH_RTLD=\"${BINDIR}/${PROG}\"
CPPFLAGS+= -I${.CURDIR}
CPPFLAGS+= -DDEBUG -DRTLD_LOADER
#CPPFLAGS+= -DRTLD_DEBUG -DRTLD_DEBUG_RELOC

LDADD+=	-L${DESTDIR}/${LIBDIR} -L${CLIBOBJ} -non_shared -lc_pic
DPADD+=	${LIBC_PIC}
CFLAGS+=-g

.if (${OBJECT_FMT} == "ELF")
# to be installed
INCS=	link.h
INCSDIR=/usr/include
.endif

STRIPFLAG=

.PATH: $M

${PROG}: ${OBJS} ${DPADD}
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LDADD}
.endif

MKMAN=	no

.include <bsd.prog.mk>
