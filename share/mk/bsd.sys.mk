#	$NetBSD: bsd.sys.mk,v 1.48 2001/07/04 17:46:06 scw Exp $
#
# Overrides used for NetBSD source tree builds.

.if defined(WARNS)
.if ${WARNS} > 0
CFLAGS+= -Wall -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
# XXX Delete -Wuninitialized by default for now -- the compiler doesn't
# XXX always get it right.
CFLAGS+= -Wno-uninitialized
.endif
.if ${WARNS} > 1
CFLAGS+=-Wreturn-type -Wcast-qual -Wpointer-arith -Wwrite-strings
CFLAGS+=-Wswitch -Wshadow
.endif
.endif

.if defined(WFORMAT) && defined(FORMAT_AUDIT)
.if ${WFORMAT} > 1
CFLAGS+=-Wnetbsd-format-audit -Wno-format-extra-args
.endif
.endif

.if !defined(NOGCCERROR)
CFLAGS+= -Werror
.endif
CFLAGS+= ${CWARNFLAGS}

.if defined(DESTDIR)
CPPFLAGS+= -nostdinc -idirafter ${DESTDIR}/usr/include
LINTFLAGS+= -d ${DESTDIR}/usr/include
.endif

.if defined(AUDIT)
CPPFLAGS+= -D__AUDIT__
.endif

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
COPTS+=		-msoft-float
FOPTS+=		-msoft-float
.endif

# Helpers for cross-compiling
HOST_CC?=	cc
HOST_CFLAGS?=	-O
HOST_COMPILE.c?=${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} -c
HOST_LINK.c?=	${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} ${HOST_LDFLAGS}

HOST_CPP?=	cpp
HOST_CPPFLAGS?=

HOST_LD?=	ld
HOST_LDFLAGS?=

OBJCOPY?=	objcopy
STRIP?=		strip
CONFIG?=	config
RPCGEN?=	rpcgen
MKLOCALE?=	mklocale

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


.if defined(PARALLEL) || defined(LPREFIX)
LPREFIX?=yy
LFLAGS+=-P${LPREFIX}
# Lex
.l:
	${LEX.l} -o${.TARGET:R}.${LPREFIX}.c ${.IMPSRC}
	${LINK.c} -o ${.TARGET} ${.TARGET:R}.${LPREFIX}.c ${LDLIBS} -ll
	rm -f ${.TARGET:R}.${LPREFIX}.c
.l.c:
	${LEX.l} -o${.TARGET} ${.IMPSRC}
.l.o:
	${LEX.l} -o${.TARGET:R}.${LPREFIX}.c ${.IMPSRC}
	${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.${LPREFIX}.c 
	rm -f ${.TARGET:R}.${LPREFIX}.c
.l.lo:
	${LEX.l} -o${.TARGET:R}.${LPREFIX}.c ${.IMPSRC}
	${HOST_COMPILE.c} -o ${.TARGET} ${.TARGET:R}.${LPREFIX}.c 
	rm -f ${.TARGET:R}.${LPREFIX}.c
.endif

# Yacc
.if defined(YHEADER) || defined(YPREFIX)
.if defined(YPREFIX)
YFLAGS+=-p${YPREFIX}
.endif
.if defined(YHEADER)
YFLAGS+=-d
.endif
.y:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${LINK.c} -o ${.TARGET} ${.TARGET:R}.tab.c ${LDLIBS}
	rm -f ${.TARGET:R}.tab.c ${.TARGET:R}.tab.h
.y.h: ${.TARGET:R}.c
.y.c:
	${YACC.y} -o ${.TARGET} ${.IMPSRC}
.y.o:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
	rm -f ${.TARGET:R}.tab.c ${TARGET:R}.tab.h
.y.lo:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${HOST_COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
	rm -f ${.TARGET:R}.tab.c ${TARGET:R}.tab.h
.elif defined(PARALLEL)
.y:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${LINK.c} -o ${.TARGET} ${.TARGET:R}.tab.c ${LDLIBS}
	rm -f ${.TARGET:R}.tab.c
.y.c:
	${YACC.y} -o ${.TARGET} ${.IMPSRC}
.y.o:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
	rm -f ${.TARGET:R}.tab.c
.y.lo:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${HOST_COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
	rm -f ${.TARGET:R}.tab.c
.endif
