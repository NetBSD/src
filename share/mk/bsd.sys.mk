#	$NetBSD: bsd.sys.mk,v 1.56 2001/11/01 07:17:17 lukem Exp $
#
# Overrides used for NetBSD source tree builds.

.if defined(WARNS)
.if ${WARNS} > 0
CFLAGS+= -Wall -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
#CFLAGS+=-Wmissing-declarations -Wredundant-decls -Wnested-externs
# XXX Delete -Wuninitialized by default for now -- the compiler doesn't
# XXX always get it right.
CFLAGS+= -Wno-uninitialized
.endif
.if ${WARNS} > 1
CFLAGS+=-Wreturn-type -Wpointer-arith -Wwrite-strings -Wswitch -Wshadow
.endif
.if ${WARNS} > 2
CFLAGS+=-Wcast-qual
.endif
.endif

.if defined(WFORMAT) && defined(FORMAT_AUDIT)
.if ${WFORMAT} > 1
CFLAGS+=-Wnetbsd-format-audit -Wno-format-extra-args
.endif
.endif

CPPFLAGS+=	${AUDIT:D-D__AUDIT__} \
		${DESTDIR:D-nostdinc -idirafter ${DESTDIR}/usr/include}

CFLAGS+=	${NOGCCERROR:U-Werror} ${CWARNFLAGS}

LINTFLAGS+=	${DESTDIR:D-d ${DESTDIR}/usr/include}

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
COPTS+=		-msoft-float
FOPTS+=		-msoft-float
.endif

# Helpers for cross-compiling
HOST_CC?=	cc
HOST_CFLAGS?=	-O
HOST_COMPILE.c?=${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} -c
HOST_LINK.c?=	${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} ${HOST_LDFLAGS}

HOST_CXX?=	c++
HOST_CXXFLAGS?=	-O

HOST_CPP?=	cpp
HOST_CPPFLAGS?=

HOST_LD?=	ld
HOST_LDFLAGS?=

OBJCOPY?=	objcopy
STRIP?=		strip
CONFIG?=	config
RPCGEN?=	rpcgen
MKLOCALE?=	mklocale
MTREE?=		mtree

.SUFFIXES:	.m .o .ln .lo

# Objective C
# (Defined here rather than in <sys.mk> because `.m' is not just
#  used for Objective C source)
.m:
	${LINK.m} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.m.o:
	${COMPILE.m} ${.IMPSRC}

# Host-compiled C objects
.c.lo:
	${HOST_COMPILE.c} -o ${.TARGET} ${.IMPSRC}

# Lex
LPREFIX?=	yy
LFLAGS+=	-P${LPREFIX}

.l.o: # remove to force use of .l.c->.c.o transforms
.l:
	${LEX.l} -o${.TARGET:R}.${LPREFIX}.c ${.IMPSRC}
	${LINK.c} -o ${.TARGET} ${.TARGET:R}.${LPREFIX}.c ${LDLIBS} -ll
	rm -f ${.TARGET:R}.${LPREFIX}.c
.l.c:
	${LEX.l} -o${.TARGET} ${.IMPSRC}

# Yacc
YFLAGS+=	${YPREFIX:D-p${YPREFIX}} ${YHEADER:D-d}

.y.o: # remove to force use of .y.c->.c.o transforms
.y:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${LINK.c} -o ${.TARGET} ${.TARGET:R}.tab.c ${LDLIBS}
	rm -f ${.TARGET:R}.tab.[ch]
.y.c:
	${YACC.y} -o ${.TARGET} ${.IMPSRC}

.ifdef YHEADER
.y.h: ${.TARGET:.h=.c}
.endif
