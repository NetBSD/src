#	$NetBSD: Makefile.inc,v 1.14 2002/08/19 09:41:33 lukem Exp $

# sys sources
.PATH: ${.CURDIR}/sys

# stubs providing an entry "foo" but calling "__posix_foo()"
PSEUDO=		chown.S fchown.S lchown.S rename.S

SRCS+=		${PSEUDO}
CLEANFILES+=	${PSEUDO}

${PSEUDO}: ${ARCHDIR}/SYS.h ${DESTDIR}/usr/include/sys/syscall.h
	@printf '#include "SYS.h"\nPSEUDO(${.PREFIX},${.PREFIX:S/^/__posix_/})\n' \
		>${.TARGET}

# Lint stubs for all automatically-generated assembly stubs (GENERATED)
.if ${MKLINT} != "no"
LSRCS+=		LintSysPseudo.c
DPSRCS+=	LintSysPseudo.c
.endif

LintSysPseudo.c: ${NETBSDSRCDIR}/lib/libc/sys/makelintstub \
    ${DESTDIR}/usr/include/sys/syscall.h
	sh ${NETBSDSRCDIR}/lib/libc/sys/makelintstub -o ${.TARGET} -p \
	    -s ${DESTDIR}/usr/include/sys/syscall.h ${PSEUDO}
