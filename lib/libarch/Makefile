#	$NetBSD: Makefile,v 1.24.8.2 2014/08/20 00:02:08 tls Exp $

.include <bsd.own.mk>

# These should always be a MACHINE_CPU value
ARCHDIRS= alpha arm i386 m68k sparc x86_64

.for arch in ${ARCHDIRS}
.include "${.CURDIR}/${arch}/Makefile.inc"
.PATH:	${.CURDIR}/${arch}
.endfor

OBJS+= ${ASM}
POBJS+=	${ASM:.o=.po}
CLEANFILES+= ${ASM} ${POBJS}

LIBC_MACHINE_CPU?=	${MACHINE_CPU}

# Build the library if SRCS is now defined ...
.if defined(SRCS)
.if defined(MLIBDIR) && ${MLIBDIR} != "none" && ${LIBC_MACHINE_CPU} != "arm"
LIB=	${MLIBDIR}
.else
LIB=	${LIBC_MACHINE_CPU}
.endif
SHLIB_VERSION_FILE= ${.CURDIR}/${LIB}/shlib_version

.include <bsd.lib.mk>
.else
# ... otherwise just build the manpages
.include <bsd.man.mk>
.endif
.include <bsd.subdir.mk>
