#	$NetBSD: bsd.sys.mk,v 1.12.2.2 1998/11/09 08:07:42 cgd Exp $
#
# Overrides used for NetBSD source tree builds.

CFLAGS+= -Werror
.if defined(WARNS) && ${WARNS} == 1
CFLAGS+= -Wall -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
.endif

.if defined(DESTDIR)
CPPFLAGS+= -nostdinc -idirafter ${DESTDIR}/usr/include
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


# Objective C
# (Defined here rather than in <sys.mk> because `.m' is not just
#  used for Objective C source)
.SUFFIXES:	.m .o .ln

.m:
	${LINK.m} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}

.m.o:
	${COMPILE.m} ${.IMPSRC}


.if defined(PARALLEL)
# Lex
.l:
	${LEX.l} -o${.TARGET:R}.yy.c ${.IMPSRC}
	${LINK.c} -o ${.TARGET} ${.TARGET:R}.yy.c ${LDLIBS} -ll
	rm -f ${.TARGET:R}.yy.c
.l.c:
	${LEX.l} -o${.TARGET} ${.IMPSRC}
.l.o:
	${LEX.l} -o${.TARGET:R}.yy.c ${.IMPSRC}
	${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.yy.c 
	rm -f ${.TARGET:R}.yy.c

# Yacc
.y:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${LINK.c} -o ${.TARGET} ${.TARGET:R}.tab.c ${LDLIBS}
	rm -f ${.TARGET:R}.tab.c
.y.c:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	mv ${.TARGET:R}.tab.c ${.TARGET}
.y.o:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
	rm -f ${.TARGET:R}.tab.c
.endif
