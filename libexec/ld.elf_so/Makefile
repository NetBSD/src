#	$NetBSD: Makefile,v 1.61 2003/03/25 13:11:53 scw Exp $

.include <bsd.own.mk>			# for OBJECT_FMT definition
.include <bsd.shlib.mk>			# for SHLINKINSTALLDIR definition

ARCHSUBDIR= ${MACHINE_CPU}
M= ${.CURDIR}/arch/${ARCHSUBDIR}

.if ((${MACHINE_ARCH} == "alpha") ||					\
     (${MACHINE_ARCH} == "arm") || (${MACHINE_ARCH} == "armeb") ||	\
     (${MACHINE_ARCH} == "hppa") ||					\
     (${MACHINE_ARCH} == "i386") ||					\
     (${MACHINE_ARCH} == "m68k") ||					\
     (${MACHINE_ARCH} == "mipsel") || (${MACHINE_ARCH} == "mipseb") ||	\
     (${MACHINE_ARCH} == "powerpc") ||					\
     (${MACHINE_ARCH} == "sh3eb") || (${MACHINE_ARCH} == "sh3el") ||	\
     (${MACHINE_ARCH} == "sh5eb") || (${MACHINE_ARCH} == "sh5el") ||	\
     (${MACHINE_ARCH} == "sparc") ||					\
     (${MACHINE_ARCH} == "sparc64") ||					\
     (${MACHINE_ARCH} == "x86_64") ||					\
     (${MACHINE_ARCH} == "vax")) &&					\
    ${OBJECT_FMT} == "ELF" && ${MKPIC} != "no"

PROG=	ld.elf_so

# Adds SRCS, CPPFLAGS, LDFLAGS, etc.  Must go first so MD startup source
# is first.
.if exists($M/Makefile.inc)
.include "$M/Makefile.inc"
.endif

CLIBOBJ!=	cd ${NETBSDSRCDIR}/lib/libc && ${PRINTOBJDIR}

SRCS+=	rtld.c reloc.c symbol.c malloc.c xmalloc.c xprintf.c debug.c \
	map_object.c load.c search.c headers.c paths.c

BINDIR=${SHLINKINSTALLDIR}

CPPFLAGS+= -DLIBDIR=\"${LIBDIR}\" -D_PATH_RTLD=\"${BINDIR}/${PROG}\"
CPPFLAGS+= -I${.CURDIR}
CPPFLAGS+= -DRTLD_LOADER
CPPFLAGS+= -D_RTLD_SOURCE
#CPPFLAGS+= -DDEBUG
#CPPFLAGS+= -DRTLD_DEBUG
#CPPFLAGS+= -DRTLD_DEBUG_RELOC
#DBG=	-g
DBG=	-O3 -fomit-frame-pointer

.if ${SHLIBDIR} != ${LIBDIR}
CPPFLAGS+= -DRTLD_DEFAULT_LIBRARY_PATH=\"${SHLIBDIR}:${LIBDIR}\"
.endif

LDADD+=	-L${CLIBOBJ} -L${DESTDIR}${LIBDIR} -non_shared -lc_pic
.if ${MKPICINSTALL} != "no" 
DPADD+=	${LIBC_PIC}
.endif
DPADD+=	${CLIBOBJ}/libc_pic.a

STRIPFLAG=

.PATH: $M

${PROG}: ${OBJS} ${DPADD}
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LDADD}

.if ${SHLINKINSTALLDIR} != "/usr/libexec"
SYMLINKS+=	${SHLINKINSTALLDIR}/${PROG} /usr/libexec/${PROG}
.endif

.include <bsd.prog.mk>
.else

MAN=	ld.elf_so.1

.include <bsd.man.mk>
.endif
