#	$NetBSD: Makefile,v 1.14 2001/01/14 06:35:57 christos Exp $

PROG=	doexec
MKMAN=	no

LDSTATIC= -static		# only static compilation makes sense here

.include <bsd.own.mk>		# May define DESTDIR in mk.conf..
.if defined(DESTDIR)
LDLIBS+= -lgcc -lc -lgcc ${LIBCRTEND}
GOODAOUT_LIB= -nostdlib -L${DESTDIR}/usr/lib ${LDSTATIC} 
GOODAOUT_LIB+= ${LIBCRT0} ${LIBCRTBEGIN}
.else
GOODAOUT_LIB= ${LDSTATIC}
.endif

RP=	${.OBJDIR}/${PROG}
TD=	${.CURDIR}/tests
OD=	${.CURDIR}/good

TESTSCRIPTS=empty nonexistshell devnullscript badinterplen goodscript \
	scriptarg scriptarg-nospace

all:	${PROG} goodaout truncaout ${TESTSCRIPTS}

.for x in ${TESTSCRIPTS}
${x}: ${TD}/${x}
	cp ${TD}/${x} ${.OBJDIR}
	chmod +x ${.OBJDIR}/${x}
.endfor

CLEANFILES+= goodaout truncaout ${TESTSCRIPTS}

regress:	test-empty test-nonexist \
		test-nonexistshell test-devnullscript test-badinterplen \
		test-goodscript test-scriptarg test-scriptarg-nospace \
		test-goodaout test-truncaout

test-empty:	${PROG} empty
	${RP} ${.OBJDIR}/empty | diff - ${OD}/empty

test-nonexist:	${PROG}
	${RP} ${TD}/nonexistent | diff - ${OD}/nonexistent

test-nonexistshell: ${PROG} nonexistshell
	${RP} ${.OBJDIR}/nonexistshell | diff - ${OD}/nonexistshell

test-devnullscript: ${PROG} devnullscript
	${RP} ${.OBJDIR}/devnullscript | diff - ${OD}/devnullscript

test-badinterplen: ${PROG} badinterplen
	${RP} ${.OBJDIR}/badinterplen | diff - ${OD}/badinterplen

test-goodscript: ${PROG} goodscript
	${RP} ${.OBJDIR}/goodscript | diff - ${OD}/goodscript

test-scriptarg: ${PROG} scriptarg
	${RP} ${.OBJDIR}/scriptarg 2>&1 | diff - ${OD}/scriptarg

test-scriptarg-nospace: ${PROG} scriptarg-nospace
	${RP} ${.OBJDIR}/scriptarg-nospace 2>&1 | diff - ${OD}/scriptarg-nospace

goodaout: ${TD}/goodaout.c
	${LINK.c} ${GOODAOUT_LIB} -o ${.TARGET} ${TD}/goodaout.c ${LDLIBS}

test-goodaout: ${PROG} goodaout
	${RP} ${.OBJDIR}/goodaout | diff - ${OD}/goodaout

truncaout: goodaout
	/bin/rm -rf truncaout
	dd if=${.OBJDIR}/goodaout of=truncaout bs=16 count=1
	chmod a+x truncaout

test-truncaout: ${PROG} truncaout
	${RP} ${.OBJDIR}/truncaout | diff - ${OD}/truncaout

.include <bsd.prog.mk>
