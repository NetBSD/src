#	$NetBSD: Makefile,v 1.35 2001/01/14 09:16:47 christos Exp $

.include <bsd.own.mk>			# for OBJECT_FMT definition

ARCHSUBDIR= ${MACHINE_ARCH:C/mipse[bl]/mips/}
M= ${.CURDIR}/arch/${ARCHSUBDIR}

.if ((${MACHINE_ARCH} == "alpha") ||					\
     (${MACHINE_ARCH} == "i386") ||					\
     (${MACHINE_ARCH} == "m68k") ||					\
     (${MACHINE_ARCH} == "mipsel") || (${MACHINE_ARCH} == "mipseb") ||	\
     (${MACHINE_ARCH} == "powerpc") ||					\
     (${MACHINE_ARCH} == "sparc") ||					\
     (${MACHINE_ARCH} == "sparc64") ||					\
     (${MACHINE_ARCH} == "vax")) &&					\
    ${OBJECT_FMT} == "ELF" && ${MKPIC} != "no"

PROG=	ld.elf_so

# Adds SRCS, CPPFLAGS, LDFLAGS, etc.  Must go first so MD startup source
# is first.
.if exists($M/Makefile.inc)
.include "$M/Makefile.inc"
.endif

SRCS+=	rtld.c reloc.c symbol.c malloc.c xmalloc.c xprintf.c debug.c \
	map_object.c load.c search.c headers.c paths.c
CPPFLAGS+= -Wall -DLIBDIR=\"${LIBDIR}\" -D_PATH_RTLD=\"${BINDIR}/${PROG}\"
CPPFLAGS+= -I${.CURDIR}
CPPFLAGS+= -DDEBUG -DRTLD_LOADER
CPPFLAGS+= -D_RTLD_SOURCE
#CPPFLAGS+= -DRTLD_DEBUG
#CPPFLAGS+= -DRTLD_DEBUG_RELOC
#DBG=	-g

LDADD+=	-non_shared -L${DESTDIR}/${LIBDIR} -lc_pic
DPADD+=	${LIBC_PIC}

STRIPFLAG=

.PATH: $M

${PROG}: .NOTMAIN ${SRCS:S/.c/.o/g} ${DPADD}
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LDADD}

.endif

MKMAN=	no

.include <bsd.prog.mk>
