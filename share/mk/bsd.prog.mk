#	$NetBSD: bsd.prog.mk,v 1.81 1997/05/09 13:40:38 mycroft Exp $
#	@(#)bsd.prog.mk	8.2 (Berkeley) 4/2/94

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.include <bsd.own.mk>

.MAIN:		all
.PHONY:		cleanprog proginstall scriptsinstall
realinstall:	proginstall scriptsinstall
clean cleandir:	cleanprog

.SUFFIXES: .out .o .c .cc .C .y .l .s .8 .7 .6 .5 .4 .3 .2 .1 .0

CFLAGS+=	${COPTS}

# ELF platforms depend on crtbegin.o and crtend.o
.if (${MACHINE_ARCH} == "alpha")   || \
    (${MACHINE_ARCH} == "powerpc")
LIBCRTBEGIN?=	${DESTDIR}/usr/lib/crtbegin.o
LIBCRTEND?=	${DESTDIR}/usr/lib/crtend.o
.else
LIBCRTBEGIN?=
LIBCRTEND?=
.endif

LIBCRT0?=	${DESTDIR}/usr/lib/crt0.o
LIBC?=		${DESTDIR}/usr/lib/libc.a
LIBC_PIC?=	${DESTDIR}/usr/lib/libc_pic.a
LIBCOMPAT?=	${DESTDIR}/usr/lib/libcompat.a
LIBCRYPT?=	${DESTDIR}/usr/lib/libcrypt.a
LIBCURSES?=	${DESTDIR}/usr/lib/libcurses.a
LIBDBM?=	${DESTDIR}/usr/lib/libdbm.a
LIBDES?=	${DESTDIR}/usr/lib/libdes.a
LIBEDIT?=	${DESTDIR}/usr/lib/libedit.a
LIBGCC?=	${DESTDIR}/usr/lib/libgcc.a
LIBGNUMALLOC?=	${DESTDIR}/usr/lib/libgnumalloc.a
LIBKDB?=	${DESTDIR}/usr/lib/libkdb.a
LIBKRB?=	${DESTDIR}/usr/lib/libkrb.a
LIBKVM?=	${DESTDIR}/usr/lib/libkvm.a
LIBL?=		${DESTDIR}/usr/lib/libl.a
LIBM?=		${DESTDIR}/usr/lib/libm.a
LIBMP?=		${DESTDIR}/usr/lib/libmp.a
LIBNTP?=	${DESTDIR}/usr/lib/libntp.a
LIBPC?=		${DESTDIR}/usr/lib/libpc.a
LIBPCAP?=	${DESTDIR}/usr/lib/libpcap.a
LIBPLOT?=	${DESTDIR}/usr/lib/libplot.a
LIBRESOLV?=	${DESTDIR}/usr/lib/libresolv.a
LIBRPCSVC?=	${DESTDIR}/usr/lib/librpcsvc.a
LIBSKEY?=	${DESTDIR}/usr/lib/libskey.a
LIBTERMCAP?=	${DESTDIR}/usr/lib/libtermcap.a
LIBTELNET?=	${DESTDIR}/usr/lib/libtelnet.a
LIBUTIL?=	${DESTDIR}/usr/lib/libutil.a
LIBWRAP?=	${DESTDIR}/usr/lib/libwrap.a
LIBY?=		${DESTDIR}/usr/lib/liby.a
LIBZ?=		${DESTDIR}/usr/lib/libz.a

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
.if defined(DESTDIR)

${PROG}: ${LIBCRT0} ${DPSRCS} ${OBJS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${DPADD}
	${CC} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} -nostdlib -L${DESTDIR}/usr/lib ${LIBCRT0} ${LIBCRTBEGIN} ${OBJS} ${LDADD} -lgcc -lc -lgcc ${LIBCRTEND}

.else

${PROG}: ${LIBCRT0} ${DPSRCS} ${OBJS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${DPADD}
	${CC} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${OBJS} ${LDADD}

.endif	# defined(DESTDIR)
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
.include <bsd.links.mk>
.include <bsd.files.mk>
.include <bsd.inc.mk>
.include <bsd.dep.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
