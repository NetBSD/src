#	$NetBSD: Makefile,v 1.18 2003/09/30 04:20:24 mycroft Exp $

NOMAN=		# defined
LDSTATIC=	-static		# only static compilation makes sense here

PROG=		doexec

.include <bsd.own.mk>		# May define DESTDIR in mk.conf..
.if defined(DESTDIR)
LDLIBS+=	-lgcc -lc -lgcc ${LIBCRTEND}
GOODAOUT_LIB=	-nostdlib -L${DESTDIR}/usr/lib ${LDSTATIC} \
		${LIBCRT0} ${LIBCRTBEGIN}
.else
GOODAOUT_LIB=	${LDSTATIC}
.endif

RP=		./${PROG}
TD=		${.CURDIR}/tests
OD=		${.CURDIR}/good

TESTSCRIPTS=	empty nonexistshell devnullscript badinterplen goodscript \
		scriptarg scriptarg-nospace

all:		${PROG} goodaout truncaout ${TESTSCRIPTS}

.for x in ${TESTSCRIPTS}
${x}: ${TD}/${x}
	cp ${TD}/${x} .
	chmod +x ${x}
.endfor

CLEANFILES+=	goodaout truncaout ${TESTSCRIPTS}

regress:	test-empty test-nonexist \
		test-nonexistshell test-devnullscript test-badinterplen \
		test-goodscript test-scriptarg test-scriptarg-nospace \
		test-goodaout test-truncaout

test-empty:	${PROG} empty
	${RP} empty | diff ${OD}/empty -

test-nonexist:	${PROG}
	${RP} ${TD}/nonexistent | diff ${OD}/nonexistent -

test-nonexistshell: ${PROG} nonexistshell
	${RP} nonexistshell | diff ${OD}/nonexistshell -

test-devnullscript: ${PROG} devnullscript
	${RP} devnullscript | diff ${OD}/devnullscript -

test-badinterplen: ${PROG} badinterplen
	${RP} badinterplen | diff ${OD}/badinterplen -

test-goodscript: ${PROG} goodscript
	${RP} goodscript | diff ${OD}/goodscript -

test-scriptarg: ${PROG} scriptarg
	${RP} scriptarg 2>&1 | diff ${OD}/scriptarg -

test-scriptarg-nospace: ${PROG} scriptarg-nospace
	${RP} scriptarg-nospace 2>&1 | diff ${OD}/scriptarg-nospace -

goodaout: ${TD}/goodaout.c
	${LINK.c} ${GOODAOUT_LIB} -o ${.TARGET} ${TD}/goodaout.c ${LDLIBS}

test-goodaout: ${PROG} goodaout
	${RP} goodaout | diff ${OD}/goodaout -

truncaout: goodaout
	/bin/rm -rf truncaout
	dd if=goodaout of=truncaout bs=16 count=1
	chmod a+x truncaout

test-truncaout: ${PROG} truncaout
	${RP} truncaout | diff ${OD}/truncaout -

.include <bsd.prog.mk>
