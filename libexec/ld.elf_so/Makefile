#	$NetBSD: Makefile,v 1.21 1999/02/21 18:10:24 kleink Exp $

.include <bsd.own.mk>			# for OBJECT_FMT definition

.if ${OBJECT_FMT} == "ELF" && ${MKPIC} != "no"
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
#CPPFLAGS+= -DRTLD_DEBUG -DRTLD_DEBUG_RELOC

LDADD+=	-L${DESTDIR}/${LIBDIR} -L${CLIBOBJ} -non_shared -lc_pic
DPADD+=	${LIBC_PIC}

.if (${OBJECT_FMT} == "ELF")
# to be installed
INCS=	link.h
INCSDIR=/usr/include
.endif

STRIPFLAG=

.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}

${PROG}: ${OBJS} ${DPADD}
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LDADD}
.endif

MKMAN=	no

.include <bsd.prog.mk>
