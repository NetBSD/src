#	$NetBSD: bsd.prog.mk,v 1.104.4.1 1999/12/27 18:31:11 wrstuden Exp $
#	@(#)bsd.prog.mk	8.2 (Berkeley) 4/2/94

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.depall.mk>
.MAIN:		all
.endif

.PHONY:		cleanprog proginstall scriptsinstall
realinstall:	proginstall scriptsinstall
clean cleandir distclean: cleanprog

CFLAGS+=	${COPTS}

# ELF platforms depend on crtbegin.o and crtend.o
.if ${OBJECT_FMT} == "ELF"
LIBCRTBEGIN?=	${DESTDIR}/usr/lib/crtbegin.o
LIBCRTEND?=	${DESTDIR}/usr/lib/crtend.o
.else
LIBCRTBEGIN?=
LIBCRTEND?=
.endif

LIBCRT0?=	${DESTDIR}/usr/lib/crt0.o

LIBBZ2?=	${DESTDIR}/usr/lib/libbz2.a
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
LIBIPSEC?=	${DESTDIR}/usr/lib/libipsec.a
LIBKDB?=	${DESTDIR}/usr/lib/libkdb.a
LIBKRB?=	${DESTDIR}/usr/lib/libkrb.a
LIBKVM?=	${DESTDIR}/usr/lib/libkvm.a
LIBL?=		${DESTDIR}/usr/lib/libl.a
LIBM?=		${DESTDIR}/usr/lib/libm.a
LIBMENU?=	${DESTDIR}/usr/lib/libmenu.a
LIBMP?=		${DESTDIR}/usr/lib/libmp.a
LIBNTP?=	${DESTDIR}/usr/lib/libntp.a
LIBOBJC?=	${DESTDIR}/usr/lib/libobjc.a
LIBPC?=		${DESTDIR}/usr/lib/libpc.a
LIBPCAP?=	${DESTDIR}/usr/lib/libpcap.a
LIBPLOT?=	${DESTDIR}/usr/lib/libplot.a
LIBPOSIX?=	${DESTDIR}/usr/lib/libposix.a
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
.if defined(YHEADER)
CLEANFILES+=	${SRCS:M*.y:.y=.h}
.endif

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

.if !defined(MAN)
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
.if !defined(BUILD) && !make(all) && !make(${PROG})
${DESTDIR}${BINDIR}/${PROGNAME}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${BINDIR}/${PROGNAME}
${DESTDIR}${BINDIR}/${PROGNAME}: ${PROG}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${STRIPFLAG} ${INSTPRIV} \
	    -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.ALLSRC} ${.TARGET}
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
.if !defined(BUILD) && !make(all) && !make(${S})
${DESTDIR}${SCRIPTSDIR_${S}}/${SCRIPTSNAME_${S}}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${SCRIPTSDIR_${S}}/${SCRIPTSNAME_${S}}
${DESTDIR}${SCRIPTSDIR_${S}}/${SCRIPTSNAME_${S}}: ${S}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${INSTPRIV} \
	    -o ${SCRIPTSOWN_${S}} -g ${SCRIPTSGRP_${S}} -m ${SCRIPTSMODE_${S}} \
	    ${.ALLSRC} ${.TARGET}
.endfor
.endif

.if !target(scriptsinstall)
scriptsinstall::
.endif

lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	${LINT} ${LINTFLAGS} ${LDFLAGS:M-L*} ${LOBJS} ${LDADD}
.endif

.include <bsd.man.mk>
.include <bsd.nls.mk>
.include <bsd.files.mk>
.include <bsd.inc.mk>
.include <bsd.links.mk>
.include <bsd.dep.mk>
.include <bsd.sys.mk>

# Make sure all of the standard targets are defined, even if they do nothing.
regress:
