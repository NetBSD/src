#	$NetBSD: Makefile,v 1.131 2007/05/30 01:13:15 tls Exp $
#	@(#)Makefile	8.2 (Berkeley) 2/3/94
#
# All library objects contain sccsid strings by default; they may be
# excluded as a space-saving measure.  To produce a library that does
# not contain these strings, delete -DLIBC_SCCS and -DSYSLIBC_SCCS
# from CPPFLAGS below.  To remove these strings from just the system call
# stubs, remove just -DSYSLIBC_SCCS from CPPFLAGS.
#
# The NLS (message catalog) functions are always in libc.  To choose that
# strerror(), perror(), strsignal(), psignal(), etc. actually call the NLS
# functions, put -DNLS on the CPPFLAGS line below.
#
# The YP functions are always in libc. To choose that getpwent() and friends
# actually call the YP functions, put -DYP on the CPPFLAGS line below.
#
# The Hesiod functions are always in libc. To choose that getpwent() and friends
# actually call the Hesiod functions, put -DHESIOD on the CPPFLAGS line below.

.include "Makefile.inc"

LIB=		c
CPPFLAGS+=	-I${.CURDIR}/include -I${.CURDIR}

.if exists (${ARCHDIR}/Makefile.inc)
.PATH:	${ARCHDIR}
.include "${ARCHDIR}/Makefile.inc"
.endif

.if exists (${ARCHDIR}/genassym.cf)
DPSRCS+=	assym.h
CLEANFILES+=	assym.h assym.h.tmp

assym.h: ${ARCHDIR}/genassym.cf
	${_MKTARGET_CREATE}
	${TOOL_GENASSYM} -- ${CC} ${CFLAGS} \
		${CPPFLAGS} ${CPPFLAGS.assym.h} ${PROF} \
		< ${ARCHDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h
.endif

# The following controls how to build compatibility code for old NetBSD
# binaries. If BUILDCOLD is yes, then we build a separate library; otherwise
# we include the code in libc.
BUILDCOLD?= no
.if "${BUILDCOLD}" == "yes"
SUBDIR=compat
.include <bsd.subdir.mk>
.else
COMPATDIR=${.CURDIR}/compat
.include "${.CURDIR}/compat/Makefile.inc"
.endif

.include "${.CURDIR}/../../common/lib/libc/Makefile.inc"
.include "${.CURDIR}/db/Makefile.inc"
.include "${.CURDIR}/citrus/Makefile.inc"
.include "${.CURDIR}/compat-43/Makefile.inc"
.include "${.CURDIR}/dlfcn/Makefile.inc"
.include "${.CURDIR}/gdtoa/Makefile.inc"
.include "${.CURDIR}/gen/Makefile.inc"
.include "${.CURDIR}/gmon/Makefile.inc"
.include "${.CURDIR}/hash/Makefile.inc"
.include "${.CURDIR}/iconv/Makefile.inc"
.include "${.CURDIR}/inet/Makefile.inc"
.include "${.CURDIR}/isc/Makefile.inc"
.include "${.CURDIR}/locale/Makefile.inc"
.include "${.CURDIR}/md/Makefile.inc"
.include "${.CURDIR}/net/Makefile.inc"
.include "${.CURDIR}/nameser/Makefile.inc"
.include "${.CURDIR}/nls/Makefile.inc"
.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "sparc64")
.include "${.CURDIR}/quad/Makefile.inc"
.endif
.include "${.CURDIR}/regex/Makefile.inc"
.include "${.CURDIR}/resolv/Makefile.inc"
.include "${.CURDIR}/rpc/Makefile.inc"
.include "${.CURDIR}/ssp/Makefile.inc"
.include "${.CURDIR}/stdio/Makefile.inc"
.include "${.CURDIR}/stdlib/Makefile.inc"
.include "${.CURDIR}/string/Makefile.inc"
.include "${.CURDIR}/termios/Makefile.inc"
.include "${.CURDIR}/thread-stub/Makefile.inc"
.include "${.CURDIR}/time/Makefile.inc"
.include "${.CURDIR}/sys/Makefile.inc"
.include "${.CURDIR}/uuid/Makefile.inc"
.if (${MKYP} != "no")
.include "${.CURDIR}/yp/Makefile.inc"
.endif

NLS=	C.msg Pig.msg ca.msg cs.msg de.msg es.msg fi.msg fr.msg nl.msg \
	no.msg pl.msg sk.msg sv.msg

realall: tags
tags: ${SRCS}
	${_MKTARGET_CREATE}
	-${TOOL_CTAGS} -w ${.ALLSRC:M*.c}
	-egrep "^ENTRY(.*)|^FUNC(.*)|^SYSCALL(.*)" /dev/null ${.ALLSRC:M*.S} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${.TARGET}; sort -o ${.TARGET} ${.TARGET}

FILES=		tags
FILESNAME=	libc.tags
FILESDIR=	/var/db


# workaround for I18N stuffs: build singlebyte setlocale() for libc.a,
# multibyte for libc.so.  the quirk should be removed when we support
# dlopen() from within statically linked binaries.
CSHLIBFLAGS+=	-D_I18N_DYNAMIC

.include <bsd.lib.mk>
