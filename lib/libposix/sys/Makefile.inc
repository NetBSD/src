#	$NetBSD: Makefile.inc,v 1.18 2003/08/01 17:03:59 lukem Exp $

# sys sources
.PATH: ${.CURDIR}/sys

# stubs providing an entry "foo" but calling "__posix_foo()"
PSEUDO=		chown.S fchown.S lchown.S rename.S

SRCS+=		${PSEUDO}
CLEANFILES+=	${PSEUDO}

ASMDEPS=	${.CURDIR}/sys/Makefile.inc ${ARCHDIR}/SYS.h \
		${DESTDIR}/usr/include/sys/syscall.h

${PSEUDO}: ${ASMDEPS}
	@printf '#include "SYS.h"\nPSEUDO(${.PREFIX},${.PREFIX:S/^/__posix_/})\n' \
		>${.TARGET}

# Lint stubs for all automatically-generated assembly stubs (GENERATED)
.if ${MKLINT} != "no"
LSRCS+=		LintSysPseudo.c
DPSRCS+=	LintSysPseudo.c
CLEANFILES+=	LintSysPseudo.c
.endif

LintSysPseudo.c: ${NETBSDSRCDIR}/lib/libc/sys/makelintstub \
    ${DESTDIR}/usr/include/sys/syscall.h
	CPP=${CPP:Q} ${HOST_SH} ${NETBSDSRCDIR}/lib/libc/sys/makelintstub \
	    -o ${.TARGET} -p -s ${DESTDIR}/usr/include/sys/syscall.h ${PSEUDO}
