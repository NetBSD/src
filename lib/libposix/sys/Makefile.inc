#	$NetBSD: Makefile.inc,v 1.11 1999/10/27 14:58:30 is Exp $

# sys sources
.PATH: ${.CURDIR}/sys

# stubs providing an entry "foo" but calling "__posix_foo()"
PSEUDO=	chown.o fchown.o lchown.o rename.o
PPSEUDO=${PSEUDO:.o=.po}
SPSEUDO=${PSEUDO:.o=.so}

OBJS+=	${PSEUDO}

${PPSEUDO}: ${ARCHDIR}/SYS.h \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@printf '#include "SYS.h"\n \
	    PSEUDO(${.PREFIX},${.PREFIX:S/^/__posix_/})\n' | \
	    ${CPP} -DGPROF ${CPPFLAGS:M-[ID]*} ${AINC} | ${AS} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

${SPSEUDO}: ${ARCHDIR}/SYS.h \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@printf '#include "SYS.h"\n \
	    PSEUDO(${.PREFIX},${.PREFIX:S/^/__posix_/})\n' | \
	    ${CPP} ${CPPPICFLAGS} ${CPPFLAGS:M-[ID]*} ${AINC} | \
	    ${AS} ${APICFLAGS} -o ${.TARGET}

${PSEUDO}: ${ARCHDIR}/SYS.h \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@printf '#include "SYS.h"\n \
	    PSEUDO(${.PREFIX},${.PREFIX:S/^/__posix_/})\n' | \
	    ${CPP} ${CPPFLAGS:M-[ID]*} ${AINC} | ${AS} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

# Lint stubs for all automatically-generated assembly stubs (GENERATED)
LSRCS+=	LintSysPseudo.c
DPSRCS+=LintSysPseudo.c

CLEANFILES+= LintSysPseudo.c

LintSysPseudo.c: ${.CURDIR}/../libc/sys/makelintstub \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@sh ${.CURDIR}/../libc/sys/makelintstub -o ${.TARGET} -p \
	    -s ${DESTDIR}/usr/include/sys/syscall.h ${PSEUDO}
