#	$NetBSD: bsd.sys.mk,v 1.31 1998/10/30 11:17:08 lukem Exp $
#
# Overrides used for NetBSD source tree builds.

.if defined(WARNS) && ${WARNS} == 1
CFLAGS+= -Wall -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
.endif
CFLAGS+= -Werror ${CWARNFLAGS}

.if defined(DESTDIR)
CPPFLAGS+= -nostdinc -idirafter ${DESTDIR}/usr/include
LINTFLAGS+= -d ${DESTDIR}/usr/include
.endif

.if defined(AUDIT)
CPPFLAGS+= -D__AUDIT__
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

STRIPPROG?=	strip


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


.if defined(PARALLEL) || defined(LEXPREFIX)
LEXPREFIX?=yy
LFLAGS+=-P${LEXPREFIX}
# Lex
.l:
	${LEX.l} -o${.TARGET:R}.${LEXPREFIX}.c ${.IMPSRC}
	${LINK.c} -o ${.TARGET} ${.TARGET:R}.${LEXPREFIX}.c ${LDLIBS} -ll
	rm -f ${.TARGET:R}.${LEXPREFIX}.c
.l.c:
	${LEX.l} -o${.TARGET} ${.IMPSRC}
.l.o:
	${LEX.l} -o${.TARGET:R}.${LEXPREFIX}.c ${.IMPSRC}
	${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.${LEXPREFIX}.c 
	rm -f ${.TARGET:R}.${LEXPREFIX}.c
.l.lo:
	${LEX.l} -o${.TARGET:R}.${LEXPREFIX}.c ${.IMPSRC}
	${HOST_COMPILE.c} -o ${.TARGET} ${.TARGET:R}.${LEXPREFIX}.c 
	rm -f ${.TARGET:R}.${LEXPREFIX}.c
.endif

# Yacc
.if defined(YHEADER) || defined(YACCPREFIX)
YACCPREFIX?=yy
YFLAGS+=-d -p${YACCPREFIX}
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
