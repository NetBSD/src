#	$NetBSD: Makefile.inc,v 1.1 1997/05/18 20:14:22 kleink Exp $

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
	    ${CPP} -DPROF ${CFLAGS:M-[ID]*} ${AINC} | ${AS} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.if (${MACHINE_ARCH} != "alpha")
${SPSEUDO}: ${.CURDIR}/../libc/arch/${MACHINE_ARCH}/SYS.h \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@printf '#include "SYS.h"\n \
	    PSEUDO(${.PREFIX},${.PREFIX:S/^/posix_/})\n' | \
	    ${CPP} -DPIC ${CFLAGS:M-[ID]*} ${AINC} | ${AS} -k -o ${.TARGET}
.else
${SPSEUDO}: ${.CURDIR}/../libc/arch/${MACHINE_ARCH}/SYS.h \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@printf '#include "SYS.h"\n \
	    PSEUDO(${.PREFIX},${.PREFIX:S/^/posix_/})\n' | \
	    ${CPP} -DPIC ${CFLAGS:M-[ID]*} ${AINC} | ${AS} -o ${.TARGET}
.endif

${PSEUDO}: ${.CURDIR}/../libc/arch/${MACHINE_ARCH}/SYS.h \
    ${DESTDIR}/usr/include/sys/syscall.h
	@echo creating ${.TARGET}
	@printf '#include "SYS.h"\n \
	    PSEUDO(${.PREFIX},${.PREFIX:S/^/posix_/})\n' | \
	    ${CPP} ${CFLAGS:M-[ID]*} ${AINC} | ${AS} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

${LPSEUDO}: ${DESTDIR}/usr/include/sys/syscall.h ${.CURDIR}/sys/makelintstub
	@echo creating ${.TARGET}
	@${.CURDIR}/sys/makelintstub ${.PREFIX:S/posix_//} ${.PREFIX} | \
	    ${LINT} -z ${LINTFLAGS} ${CFLAGS:M-[IDU]*} -i -o ${.TARGET} -
