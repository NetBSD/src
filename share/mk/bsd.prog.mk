#	$NetBSD: bsd.prog.mk,v 1.85 1997/05/26 03:58:41 cjs Exp $
#	@(#)bsd.prog.mk	8.2 (Berkeley) 4/2/94

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.include <bsd.own.mk>

.MAIN:		all
.PHONY:		cleanprog proginstall scriptsinstall
realinstall:	proginstall scriptsinstall
clean cleandir:	cleanprog

CFLAGS+=	${COPTS}

# ELF platforms depend on crtbegin.o and crtend.o
.if (${MACHINE_ARCH} == "alpha")   || \
    (${MACHINE_ARCH} == "powerpc")
LIBCRTBEGIN?=	${BUILDDIR}/usr/lib/crtbegin.o
LIBCRTEND?=	${BUILDDIR}/usr/lib/crtend.o
.else
LIBCRTBEGIN?=
LIBCRTEND?=
.endif

LIBCRT0?=	${BUILDDIR}/usr/lib/crt0.o
LIBC?=		${BUILDDIR}/usr/lib/libc.a
LIBC_PIC?=	${BUILDDIR}/usr/lib/libc_pic.a
LIBCOMPAT?=	${BUILDDIR}/usr/lib/libcompat.a
LIBCRYPT?=	${BUILDDIR}/usr/lib/libcrypt.a
LIBCURSES?=	${BUILDDIR}/usr/lib/libcurses.a
LIBDBM?=	${BUILDDIR}/usr/lib/libdbm.a
LIBDES?=	${BUILDDIR}/usr/lib/libdes.a
LIBEDIT?=	${BUILDDIR}/usr/lib/libedit.a
LIBGCC?=	${BUILDDIR}/usr/lib/libgcc.a
LIBGNUMALLOC?=	${BUILDDIR}/usr/lib/libgnumalloc.a
LIBKDB?=	${BUILDDIR}/usr/lib/libkdb.a
LIBKRB?=	${BUILDDIR}/usr/lib/libkrb.a
LIBKVM?=	${BUILDDIR}/usr/lib/libkvm.a
LIBL?=		${BUILDDIR}/usr/lib/libl.a
LIBM?=		${BUILDDIR}/usr/lib/libm.a
LIBMP?=		${BUILDDIR}/usr/lib/libmp.a
LIBNTP?=	${BUILDDIR}/usr/lib/libntp.a
LIBPC?=		${BUILDDIR}/usr/lib/libpc.a
LIBPCAP?=	${BUILDDIR}/usr/lib/libpcap.a
LIBPLOT?=	${BUILDDIR}/usr/lib/libplot.a
LIBRESOLV?=	${BUILDDIR}/usr/lib/libresolv.a
LIBRPCSVC?=	${BUILDDIR}/usr/lib/librpcsvc.a
LIBSKEY?=	${BUILDDIR}/usr/lib/libskey.a
LIBTERMCAP?=	${BUILDDIR}/usr/lib/libtermcap.a
LIBTELNET?=	${BUILDDIR}/usr/lib/libtelnet.a
LIBUTIL?=	${BUILDDIR}/usr/lib/libutil.a
LIBWRAP?=	${BUILDDIR}/usr/lib/libwrap.a
LIBY?=		${BUILDDIR}/usr/lib/liby.a
LIBZ?=		${BUILDDIR}/usr/lib/libz.a

.if defined(SHAREDSTRINGS)
CLEANFILES+=strings
.c.o:
	${CC} -E ${CFLAGS} ${.IMPSRC} | xstr -c -
	@${CC} ${CFLAGS} -c x.c -o ${.TARGET}
	@rm -f x.c

.cc.o:
	${CXX} -E ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.cc
	@${CXX} ${CXXFLAGS} -c x.cc -o ${.TARGET}
	@rm -f x.cc

.C.o:
	${CXX} -E ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.C
	@${CXX} ${CXXFLAGS} -c x.C -o ${.TARGET}
	@rm -f x.C
.endif


.if defined(PROG)
SRCS?=		${PROG}.c

DPSRCS+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${DPSRCS}

.if !empty(SRCS:N*.h:N*.sh)
OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.o/g}
LOBJS+=		${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
.endif

.if defined(OBJS) && !empty(OBJS)
.NOPATH: ${OBJS}
.if defined(BUILDDIR)
# link against the libs in BUILDDIR
${PROG}: ${LIBCRT0} ${DPSRCS} ${OBJS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${DPADD}
	${CC} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} -nostdlib -L${BUILDDIR}/usr/lib ${LIBCRT0} ${LIBCRTBEGIN} ${OBJS} ${LDADD} -lgcc -lc -lgcc ${LIBCRTEND}

.else
# link against libs in system we're running
${PROG}: ${LIBCRT0} ${DPSRCS} ${OBJS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${DPADD}
	${CC} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${OBJS} ${LDADD}

.endif	# defined(BUILDDIR)
.endif	# defined(OBJS) && !empty(OBJS)

.if	!defined(MAN)
MAN=	${PROG}.1
.endif	# !defined(MAN)
.endif	# defined(PROG)

all: ${PROG}

cleanprog:
	rm -f a.out [Ee]rrs mklog core *.core \
	    ${PROG} ${OBJS} ${LOBJS} ${CLEANFILES}

.if defined(SRCS)
afterdepend: .depend
	@(TMP=/tmp/_depend$$$$; \
	    sed -e 's/^\([^\.]*\).o[ ]*:/\1.o \1.ln:/' \
	      < .depend > $$TMP; \
	    mv $$TMP .depend)
.endif

.if defined(PROG) && !target(proginstall)
PROGNAME?= ${PROG}
proginstall:: ${DESTDIR}${BINDIR}/${PROGNAME}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${BINDIR}/${PROGNAME}
.endif
.if !defined(BUILD)
${DESTDIR}${BINDIR}/${PROGNAME}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${BINDIR}/${PROGNAME}
${DESTDIR}${BINDIR}/${PROGNAME}: ${PROG}
	${INSTALL} ${COPY} ${STRIPFLAG} -o ${BINOWN} -g ${BINGRP} \
	    -m ${BINMODE} ${.ALLSRC} ${.TARGET}
.endif

.if !target(proginstall)
proginstall::
.endif

.if defined(SCRIPTS) && !target(scriptsinstall)
SCRIPTSDIR?=${BINDIR}
SCRIPTSOWN?=${BINOWN}
SCRIPTSGRP?=${BINGRP}
SCRIPTSMODE?=${BINMODE}
.for S in ${SCRIPTS}
SCRIPTSDIR_${S}?=${SCRIPTSDIR}
SCRIPTSOWN_${S}?=${SCRIPTSOWN}
SCRIPTSGRP_${S}?=${SCRIPTSGRP}
SCRIPTSMODE_${S}?=${SCRIPTSMODE}
.if defined(SCRIPTSNAME)
SCRIPTSNAME_${S} ?= ${SCRIPTSNAME}
.else
SCRIPTSNAME_${S} ?= ${S:T:R}
.endif
SCRIPTSDIR_${S} ?= ${SCRIPTSDIR}
scriptsinstall:: ${DESTDIR}${SCRIPTSDIR_${S}}/${SCRIPTSNAME_${S}}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${SCRIPTSDIR_${S}}/${SCRIPTSNAME_${S}}
.endif
.if !defined(BUILD)
${DESTDIR}${SCRIPTSDIR_${S}}/${SCRIPTSNAME_${S}}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${SCRIPTSDIR_${S}}/${SCRIPTSNAME_${S}}
${DESTDIR}${SCRIPTSDIR_${S}}/${SCRIPTSNAME_${S}}: ${S}
	${INSTALL} ${COPY} -o ${SCRIPTSOWN_${S}} -g ${SCRIPTSGRP_${S}} \
		-m ${SCRIPTSMODE_${S}} ${.ALLSRC} ${.TARGET}
.endfor
.endif

.if !target(scriptsinstall)
scriptsinstall::
.endif

lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	@${LINT} ${LINTFLAGS} ${LDFLAGS:M-L*} ${LOBJS} ${LDADD}
.endif

.if !defined(NOMAN)
.include <bsd.man.mk>
.endif

.if !defined(NONLS)
.include <bsd.nls.mk>
.endif

.include <bsd.obj.mk>
.include <bsd.files.mk>
.include <bsd.inc.mk>
.include <bsd.links.mk>
.include <bsd.dep.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
