#	$NetBSD: Makefile.inc,v 1.9 1999/02/24 15:10:17 drochner Exp $

# sys sources
.PATH: ${ARCHDIR}/sys ${.CURDIR}/sys

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
	    ${CPP} -DPROF ${CPPFLAGS:M-[ID]*} ${AINC} | ${AS} -o ${.TARGET}.o
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
LSRCS+=	LintSysNormal.c LintSysNoerr.c LintSysPseudo.c
DPSRCS+= LintSysNormal.c LintSysNoerr.c LintSysPseudo.c

CLEANFILES+= LintSysNormal.c LintSysNoerr.c LintSysPseudo.c

LintSysNormal.c: ${.CURDIR}/../libc/sys/makelintstub \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@sh ${.CURDIR}/../libc/sys/makelintstub -o ${.TARGET} \
	    -s ${DESTDIR}/usr/include/sys/syscall.h ${ASM}

LintSysNoerr.c: ${.CURDIR}/../libc/sys/makelintstub \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@sh ${.CURDIR}/../libc/sys/makelintstub -o ${.TARGET} -n \
	    -s ${DESTDIR}/usr/include/sys/syscall.h ${NOERR}

LintSysPseudo.c: ${.CURDIR}/../libc/sys/makelintstub \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@sh ${.CURDIR}/../libc/sys/makelintstub -o ${.TARGET} -p \
	    -s ${DESTDIR}/usr/include/sys/syscall.h ${PSEUDO}
