#	@(#)bsd.prog.mk	5.26 (Berkeley) 6/25/91

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.SUFFIXES: .out .o .c .y .l .s .8 .7 .6 .5 .4 .3 .2 .1 .0

.8.0 .7.0 .6.0 .5.0 .4.0 .3.0 .2.0 .1.0:
	nroff -mandoc ${.IMPSRC} > ${.TARGET}

CFLAGS+=${COPTS}

STRIP?=	-s

BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	555

LIBCRT0?=	/usr/lib/crt0.o
LIBC?=		/usr/lib/libc.a
LIBCOMPAT?=	/usr/lib/libcompat.a
.ifndef EXPORTABLE_SYSTEM
LIBCRYPT?=	/usr/lib/libcrypt.a
.endif
LIBCURSES?=	/usr/lib/libcurses.a
LIBDBM?=	/usr/lib/libdbm.a
LIBDES?=	/usr/lib/libdes.a
LIBL?=		/usr/lib/libl.a
LIBKDB?=	/usr/lib/libkdb.a
LIBKRB?=	/usr/lib/libkrb.a
LIBM?=		/usr/lib/libm.a
LIBMP?=		/usr/lib/libmp.a
LIBPC?=		/usr/lib/libpc.a
LIBPLOT?=	/usr/lib/libplot.a
LIBRESOLV?=	/usr/lib/libresolv.a
LIBRPC?=	/usr/lib/librpc.a
LIBRPCSVC?=	/usr/lib/librpcsvc.a
LIBTERM?=	/usr/lib/libterm.a
LIBUTIL?=	/usr/lib/libutil.a

.if defined(SHAREDSTRINGS)
CLEANFILES+=strings
.c.o:
	${CC} -E ${CFLAGS} ${.IMPSRC} | xstr -c -
	@${CC} ${CFLAGS} -c x.c -o ${.TARGET}
	@rm -f x.c
.endif

.if defined(PROG)
.if defined(SRCS)

OBJS+=  ${SRCS:R:S/$/.o/g}

.if defined(LDONLY)

${PROG}: ${LIBCRT0} ${LIBC} ${OBJS} ${DPADD}
	${LD} ${LDFLAGS} -o ${.TARGET} ${LIBCRT0} ${OBJS} ${LIBC} ${LDADD}

.else defined(LDONLY)

${PROG}: ${OBJS} ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} -o ${.TARGET} ${OBJS} ${LDADD}

.endif

.else defined(PROG)

SRCS= ${PROG}.c

${PROG}: ${SRCS} ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} ${CFLAGS} -o ${.TARGET} ${.CURDIR}/${SRCS} ${LDADD}

MKDEP=	-p

.endif

.if	!defined(MAN1) && !defined(MAN2) && !defined(MAN3) && \
	!defined(MAN4) && !defined(MAN5) && !defined(MAN6) && \
	!defined(MAN7) && !defined(MAN8)
MAN1=	${PROG}.0
.endif
.endif

.if !defined(NOMAN)
MANALL=	${MAN1} ${MAN2} ${MAN3} ${MAN4} ${MAN5} ${MAN6} ${MAN7} ${MAN8}
.endif

_PROGSUBDIR: .USE
.if defined(SUBDIR) && !empty(SUBDIR)
	@for entry in ${SUBDIR}; do \
		(echo "===> $$entry"; \
		if test -d ${.CURDIR}/$${entry}.${MACHINE}; then \
			cd ${.CURDIR}/$${entry}.${MACHINE}; \
		else \
			cd ${.CURDIR}/$${entry}; \
		fi; \
		${MAKE} ${.TARGET:S/realinstall/install/:S/.depend/depend/}); \
	done
.endif

.MAIN: all
all: ${PROG} ${MANALL} _PROGSUBDIR

.if !target(clean)
clean: _PROGSUBDIR
	rm -f a.out [Ee]rrs mklog core ${PROG} ${OBJS} ${CLEANFILES}
.endif

.if !target(cleandir)
cleandir: _PROGSUBDIR
	rm -f a.out [Ee]rrs mklog core ${PROG} ${OBJS} ${CLEANFILES}
	rm -f .depend ${MANALL}
.endif

# some of the rules involve .h sources, so remove them from mkdep line
.if !target(depend)
depend: .depend _PROGSUBDIR
.depend: ${SRCS}
.if defined(PROG)
	mkdep ${MKDEP} ${CFLAGS:M-[ID+]*} ${.ALLSRC:M*.c}
.endif
.endif

.if !target(install)
.if !target(beforeinstall)
beforeinstall:
.if defined(DESTDIR) || defined(BINDIR)
	@if [ ! -d "${DESTDIR}${BINDIR}" ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR} ; \
                mkdir -p ${DESTDIR}${BINDIR} ; \
                chown root.wheel ${DESTDIR}${BINDIR} ; \
                chmod 755 ${DESTDIR}${BINDIR} ; \
        else \
                true ; \
        fi
.endif
.endif
.if !target(afterinstall)
afterinstall:
.endif

.if !target(realinstall)
realinstall: _PROGSUBDIR
.if defined(PROG)
	install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG} ${DESTDIR}${BINDIR}
.endif
.if defined(HIDEGAME)
	(cd ${DESTDIR}/usr/games; rm -f ${PROG}; ln -s dm ${PROG}; \
	    chown games.bin ${PROG})
.endif
.if defined(LINKS) && !empty(LINKS)
	@set ${LINKS}; \
	while test $$# -ge 2; do \
		l=${DESTDIR}$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo $$t -\> $$l; \
		rm -f $$t; \
		ln $$l $$t; \
	done; true
.endif
.endif

install: maninstall
maninstall: afterinstall
afterinstall: realinstall
realinstall: beforeinstall
.endif

.if !target(lint)
lint: ${SRCS} _PROGSUBDIR
.if defined(PROG)
	@${LINT} ${LINTFLAGS} ${CFLAGS} ${.ALLSRC} | more 2>&1
.endif
.endif

.if !target(obj)
.if defined(NOOBJ)
obj: _PROGSUBDIR
.else
obj: _PROGSUBDIR
	@cd ${.CURDIR}; rm -f obj > /dev/null 2>&1 || true; \
	here=`pwd`; subdir=`echo $$here | sed 's,^/usr/src/,,'`; \
	if test $$here != $$subdir ; then \
		dest=/usr/obj/$$subdir ; \
		echo "$$here -> $$dest"; ln -s $$dest obj; \
		if test -d /usr/obj -a ! -d $$dest; then \
			mkdir -p $$dest; \
		else \
			true; \
		fi; \
	else \
		true ; \
		dest=$$here/obj ; \
		echo "making $$here/obj" ; \
		if test ! -d obj ; then \
			mkdir $$here/obj; \
		fi ; \
	fi;
.endif
.endif

.if !target(tags)
tags: ${SRCS} _PROGSUBDIR
.if defined(PROG)
	-cd ${.CURDIR}; ctags -f /dev/stdout ${.ALLSRC} | \
	    sed "s;\${.CURDIR}/;;" > tags
.endif
.endif

.if !defined(NOMAN)
.include <bsd.man.mk>
.endif
