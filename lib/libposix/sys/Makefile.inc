#	$NetBSD: Makefile.inc,v 1.5 1997/10/23 02:59:00 lukem Exp $

# sys sources
.PATH: ${.CURDIR}/../libc/arch/${MACHINE_ARCH}/sys ${.CURDIR}/sys

# stubs providing an entry "foo" but calling "posix_foo()"
PSEUDO=	rename.o
PPSEUDO=${PSEUDO:.o=.po}
SPSEUDO=${PSEUDO:.o=.so}
LPSEUDO=${PSEUDO:.o=.ln}

OBJS+=	${PSEUDO}
LOBJS+=	${LPSEUDO}


${PPSEUDO}: ${.CURDIR}/../libc/arch/${MACHINE_ARCH}/SYS.h \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@printf '#include "SYS.h"\n \
	    PSEUDO(${.PREFIX},${.PREFIX:S/^/posix_/})\n' | \
	    ${CPP} -DPROF ${CPPFLAGS:M-[ID]*} ${AINC} | ${AS} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

${SPSEUDO}: ${.CURDIR}/../libc/arch/${MACHINE_ARCH}/SYS.h \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@printf '#include "SYS.h"\n \
	    PSEUDO(${.PREFIX},${.PREFIX:S/^/posix_/})\n' | \
	    ${CPP} ${CPPPICFLAGS} ${CPPFLAGS:M-[ID]*} ${AINC} | \
	    ${AS} ${APICFLAGS} -o ${.TARGET}

${PSEUDO}: ${.CURDIR}/../libc/arch/${MACHINE_ARCH}/SYS.h \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@printf '#include "SYS.h"\n \
	    PSEUDO(${.PREFIX},${.PREFIX:S/^/posix_/})\n' | \
	    ${CPP} ${CPPFLAGS:M-[ID]*} ${AINC} | ${AS} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

${LPSEUDO}: ${DESTDIR}/usr/include/sys/syscall.h ${.CURDIR}/sys/makelintstub
	@echo creating ${.TARGET}
	@${.CURDIR}/sys/makelintstub ${.PREFIX:S/posix_//} ${.PREFIX} | \
	    ${LINT} -z ${LINTFLAGS} ${CPPFLAGS:M-[IDU]*} -i -o ${.TARGET} -
