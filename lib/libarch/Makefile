#	$NetBSD: Makefile,v 1.27 2014/03/18 18:20:36 riastradh Exp $

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

# Build the library if SRCS is now defined ...
.if defined(SRCS)
.if defined(MLIBDIR) && ${MLIBDIR} != "none" && ${MACHINE_CPU} != "arm"
LIB=	${MLIBDIR}
.else
LIB=	${MACHINE_CPU}
.endif
SHLIB_VERSION_FILE= ${.CURDIR}/${LIB}/shlib_version

.include <bsd.lib.mk>
.else
# ... otherwise just build the manpages
.include <bsd.man.mk>
.endif
.include <bsd.subdir.mk>
