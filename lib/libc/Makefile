#	$NetBSD: Makefile,v 1.115 2004/04/07 03:49:16 ross Exp $
#	@(#)Makefile	8.2 (Berkeley) 2/3/94
#
# All library objects contain sccsid strings by default; they may be
# excluded as a space-saving measure.  To produce a library that does
# not contain these strings, delete -DLIBC_SCCS and -DSYSLIBC_SCCS
# from CPPFLAGS below.  To remove these strings from just the system call
# stubs, remove just -DSYSLIBC_SCCS from CPPFLAGS.
#
# The NLS (message catalog) functions are always in libc.  To choose that
# strerror(), perror(), strsignal(), psignal(), etc. actually call the NLS
# functions, put -DNLS on the CPPFLAGS line below.
#
# The YP functions are always in libc. To choose that getpwent() and friends
# actually call the YP functions, put -DYP on the CPPFLAGS line below.
#
# The Hesiod functions are always in libc. To choose that getpwent() and friends
# actually call the Hesiod functions, put -DHESIOD on the CPPFLAGS line below.

USE_SHLIBDIR=	yes

.include <bsd.own.mk>

LIB=		c
CPPFLAGS+=	-D_LIBC -DLIBC_SCCS -DSYSLIBC_SCCS -D_REENTRANT
CPPFLAGS+=	-I${.CURDIR}/include

.if (${USE_HESIOD} != "no")
CPPFLAGS+=	-DHESIOD
.endif

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif
CPPFLAGS+=	-DNLS

.if (${USE_YP} != "no")
CPPFLAGS+=	-DYP
.endif

.if ${MACHINE_ARCH} == "i386"
# Set lint to exit on warnings
LINTFLAGS+=	-w
.endif
# ignore 'empty translation unit' warnings.
LINTFLAGS+=	-X 272

.if exists(${.CURDIR}/arch/${MACHINE_ARCH})
ARCHSUBDIR=	${MACHINE_ARCH}
.elif exists(${.CURDIR}/arch/${MACHINE_CPU}) 
ARCHSUBDIR=	${MACHINE_CPU}
.else
.BEGIN:
	@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

ARCHDIR=	${.CURDIR}/arch/${ARCHSUBDIR}
AINC=		-I${ARCHDIR}
.if defined(DESTDIR)
AINC+=		-nostdinc -idirafter ${DESTDIR}/usr/include
.endif
CLEANFILES+=	tags

# Don't try to lint the C library against itself when creating llib-lc.ln
LLIBS=

INCSDIR=	/usr/include

.if exists (${ARCHDIR}/Makefile.inc)
.PATH:	${ARCHDIR}
.include "${ARCHDIR}/Makefile.inc"
.endif
.if exists (${ARCHDIR}/genassym.cf)
DPSRCS+=	assym.h
CLEANFILES+=	assym.h assym.h.tmp

assym.h: genassym.sh ${ARCHDIR}/genassym.cf
	${_MKTARGET_CREATE}
	${HOST_SH} ${.CURDIR}/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
		< ${ARCHDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h
.endif

.include "${.CURDIR}/db/Makefile.inc"
.include "${.CURDIR}/citrus/Makefile.inc"
.include "${.CURDIR}/compat-43/Makefile.inc"
.include "${.CURDIR}/dlfcn/Makefile.inc"
.include "${.CURDIR}/gen/Makefile.inc"
.include "${.CURDIR}/gmon/Makefile.inc"
.include "${.CURDIR}/hash/Makefile.inc"
.include "${.CURDIR}/iconv/Makefile.inc"
.include "${.CURDIR}/locale/Makefile.inc"
.include "${.CURDIR}/md/Makefile.inc"
.include "${.CURDIR}/net/Makefile.inc"
.include "${.CURDIR}/nls/Makefile.inc"
.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "sparc64")
.include "${.CURDIR}/quad/Makefile.inc"
.endif
.include "${.CURDIR}/regex/Makefile.inc"
.include "${.CURDIR}/rpc/Makefile.inc"
.include "${.CURDIR}/stdio/Makefile.inc"
.include "${.CURDIR}/stdlib/Makefile.inc"
.include "${.CURDIR}/string/Makefile.inc"
.include "${.CURDIR}/termios/Makefile.inc"
.include "${.CURDIR}/thread-stub/Makefile.inc"
.include "${.CURDIR}/time/Makefile.inc"
.include "${.CURDIR}/sys/Makefile.inc"
.if (${MKYP} != "no")
.include "${.CURDIR}/yp/Makefile.inc"
.endif

NLS=	C.msg Pig.msg ca.msg cs.msg de.msg es.msg fi.msg fr.msg nl.msg \
	no.msg pl.msg sk.msg sv.msg

LIBKERN=	${NETBSDSRCDIR}/sys/lib/libkern

KSRCS=	bcopy.c bcmp.c bswap16.c bswap32.c bswap64.c bzero.c ffs.c \
	strcat.c strchr.c strcmp.c strcpy.c strlen.c \
	strncmp.c strncpy.c strrchr.c \
	htonl.c htons.c ntohl.c ntohs.c md5c.c \
	memchr.c memcmp.c memcpy.c memmove.c memset.c \
	index.c rindex.c
.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "sparc64")
KSRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
	lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
	subdi3.c  ucmpdi2.c udivdi3.c umoddi3.c xordi3.c
KINCLUDES+=	quad/quad.h
.endif

copy-to-libkern:	copy-to-libkern-machind copy-to-libkern-machdep

copy-to-libkern-machind: ${KSRCS}
	cp -p ${.ALLSRC} ${LIBKERN}
.if defined(KINCLUDES) && !empty(KINCLUDES)
	(cd ${.CURDIR} ; cp -p ${KINCLUDES} ${LIBKERN})
.endif

copy-to-libkern-machdep: ${KMSRCS}
.if defined(KMSRCS) && !empty(KMSRCS)
	cp -p ${.ALLSRC} ${LIBKERN}/arch/${ARCHSUBDIR}
.endif
.if defined(KMINCLUDES) && !empty(KMINCLUDES)
	(cd ${.CURDIR} ; cp -p ${KMINCLUDES} ${LIBKERN}/arch/${ARCHSUBDIR})
.endif

rm-from-libkern:
	for i in ${KSRCS}; do rm -f ${LIBKERN}/$$i; done
.if defined(KMSRCS) && !empty(KMSRCS)
	for i in ${KMSRCS}; do rm -f ${LIBKERN}/arch/${ARCHSUBDIR}/$$i; done
.endif

realall: tags
tags: ${SRCS}
	-${TOOL_CTAGS} -w ${.ALLSRC:M*.c}
	-egrep "^ENTRY(.*)|^FUNC(.*)|^SYSCALL(.*)" /dev/null ${.ALLSRC:M*.S} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${.TARGET}; sort -o ${.TARGET} ${.TARGET}

FILES=		tags
FILESNAME=	libc.tags
FILESDIR=	/var/db


# workaround for I18N stuffs: build singlebyte setlocale() for libc.a,
# multibyte for libc.so.  the quirk should be removed when we support
# dlopen() from within statically linked binaries.
CSHLIBFLAGS+=	-D_I18N_DYNAMIC

.include <bsd.lib.mk>
