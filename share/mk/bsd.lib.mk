#	$NetBSD: bsd.lib.mk,v 1.87 1997/03/22 22:33:57 perry Exp $
#	@(#)bsd.lib.mk	5.26 (Berkeley) 5/2/91

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.include <bsd.own.mk>				# for 'NOPIC' definition

.if exists(${.CURDIR}/shlib_version)
SHLIB_MAJOR != . ${.CURDIR}/shlib_version ; echo $$major
SHLIB_MINOR != . ${.CURDIR}/shlib_version ; echo $$minor
.endif

.MAIN: all

# prefer .S to a .c, add .po, remove stuff not used in the BSD libraries.
# .so used for PIC object files.  .ln used for lint output files.
.SUFFIXES:
.SUFFIXES: .out .o .po .so .S .s .c .cc .C .f .y .l .ln .m4


# Set PICFLAGS to cc flags for producing position-independent code,
# if not already set.  Includes -DPIC, if required.

# Data-driven table using make variables to control  how shared libraries
# are built for different platforms and object formats.
# SHLIB_TYPE:		currently either "ELF" or "a.out".
# SHLIB_SOVERSION:  	version number to be compiled into a shared library
#                    	via -soname. Usualy ${SHLIB_MAJOR} on ELF.
#   			NetBSD/pmax used to use ${SHLIB_MAJOR}.{SHLIB-MINOR}.
# SHLIB_LDSTARTFILE:	???
# SHLIB_LDENDTILE:	??
# CPICFLAGS:	flags to compile .c files for .so objects.
# APICFLAGS:	flags to assemble .S files for .so objects.

.if (${MACHINE_ARCH} == "alpha")

SHLIB_TYPE=ELF
SHLIB_LDSTARTFILE= ${DESTDIR}/usr/lib/crtbeginS.o
SHLIB_LDENDFILE= ${DESTDIR}/usr/lib/crtendS.o
SHLIB_SOVERSION=${SHLIB_MAJOR}
CPICFLAGS ?= -fpic -DPIC
APICFLAGS ?= -DPIC

.elif (${MACHINE_ARCH} == "mips")

SHLIB_TYPE=ELF
# still use gnu-derived ld.so on pmax; don't have or need lib<>.so support.
SHLIB_LDSTARTFILE=
SHLIB_LDENDFILE=
SHLIB_SOVERSION=${SHLIB_MAJOR}

# On mips, all libs need to be compiled with ABIcalls, not just sharedlibs.
CPICFLAGS?=
APICFLAGS?=
#CPICFLAGS?= -fpic -DPIC
#APICFLAGS?= -DPIC

# so turn shlib PIC flags on for ${CPP}, ${CC}, and ${AS} as follows:
AINC+=-DPIC -DABICALLS
COPTS+=	-fPIC ${AINC}
AFLAGS+= -fPIC
AS+=	-KPIC

.else

SHLIB_TYPE=a.out
SHLIB_SOVERSION=${SHLIB_MAJOR}.${SHLIB_MINOR}
CPICFLAGS?= -fpic -DPIC
APICFLAGS?= -fpic -DPIC

.endif


CFLAGS+=	${COPTS}

.c.o:
	@echo ${COMPILE.c:Q} ${.IMPSRC}
	@${COMPILE.c} ${.IMPSRC}  -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.c.po:
	@echo ${COMPILE.c:Q} -pg ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.c} -pg ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.c.so:
	@echo ${COMPILE.c:Q} ${CPICFLAGS} ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.c} ${CPICFLAGS} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.c.ln:
	${LINT} ${LINTFLAGS} ${CFLAGS:M-[IDU]*} -i ${.IMPSRC}

.cc.o .C.o:
	@echo ${COMPILE.cc:Q} ${.IMPSRC}
	@${COMPILE.cc} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.cc.po .C.po:
	@echo ${COMPILE.cc:Q} -pg ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.cc} -pg ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.cc.so .C.so:
	@echo ${COMPILE.cc:Q} ${CPICFLAGS} ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.cc} ${CPICFLAGS} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.S.o .s.o:
	@echo ${COMPILE.S:Q} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC}
	@${COMPILE.S} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.S.po .s.po:
	@echo ${COMPILE.S:Q} -DGPROF -DPROF ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.S} -DGPROF -DPROF ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.S.so .s.so:
	@echo ${COMPILE.S:Q} ${APICFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.S} ${APICFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o


.if !defined(NOPROFILE)
_LIBS=lib${LIB}.a lib${LIB}_p.a
.else
_LIBS=lib${LIB}.a
.endif

.if !defined(NOPIC)
_LIBS+=lib${LIB}_pic.a
.if defined(SHLIB_MAJOR) && defined(SHLIB_MINOR)
_LIBS+=lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
.endif
.endif

.if !defined(NOLINT)
_LIBS+=llib-l${LIB}.ln
.endif

all: ${_LIBS} _SUBDIRUSE

OBJS+=	${SRCS:N*.h:R:S/$/.o/g}

lib${LIB}.a:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.a
	@${AR} cq lib${LIB}.a `lorder ${OBJS} | tsort -q`
	${RANLIB} lib${LIB}.a

POBJS+=	${OBJS:.o=.po}
lib${LIB}_p.a:: ${POBJS}
	@echo building profiled ${LIB} library
	@rm -f lib${LIB}_p.a
	@${AR} cq lib${LIB}_p.a `lorder ${POBJS} | tsort -q`
	${RANLIB} lib${LIB}_p.a

SOBJS+=	${OBJS:.o=.so}
lib${LIB}_pic.a:: ${SOBJS}
	@echo building shared object ${LIB} library
	@rm -f lib${LIB}_pic.a
	@${AR} cq lib${LIB}_pic.a `lorder ${SOBJS} | tsort -q`
	${RANLIB} lib${LIB}_pic.a

lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}: lib${LIB}_pic.a ${DPADD}
	@echo building shared ${LIB} library \(version ${SHLIB_MAJOR}.${SHLIB_MINOR}\)
	@rm -f lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
.if (${SHLIB_TYPE} == "a.out")
	$(LD) -x -Bshareable -Bforcearchive \
	    -o lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} lib${LIB}_pic.a ${LDADD}
.elif (${SHLIB_TYPE} == "ELF")
	$(LD) -x -shared -o lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	    -soname lib${LIB}.so.${SHLIB_SOVERSION}  ${SHLIB_LDSTARTFILE} \
	    --whole-archive lib${LIB}_pic.a --no-whole-archive ${LDADD} \
	    ${SHLIB_LDENDFILE}
.endif

LOBJS+=	${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
LLIBS?=	-lc
llib-l${LIB}.ln: ${LOBJS}
	@echo building llib-l${LIB}.ln
	@rm -f llib-l${LIB}.ln
	@${LINT} -C${LIB} ${LOBJS} ${LLIBS}

.if !target(clean)
cleanlib:
	rm -f a.out [Ee]rrs mklog core *.core ${CLEANFILES}
	rm -f lib${LIB}.a ${OBJS}
	rm -f lib${LIB}_p.a ${POBJS}
	rm -f lib${LIB}_pic.a lib${LIB}.so.*.* ${SOBJS}
	rm -f llib-l${LIB}.ln ${LOBJS}

clean: _SUBDIRUSE cleanlib
cleandir: _SUBDIRUSE cleanlib
.else
cleandir: _SUBDIRUSE clean
.endif

.if defined(SRCS)
afterdepend: .depend
	@(TMP=/tmp/_depend$$$$; \
	    sed -e 's/^\([^\.]*\).o[ ]*:/\1.o \1.po \1.so \1.ln:/' \
	      < .depend > $$TMP; \
	    mv $$TMP .depend)
.endif

.if !target(install)
.if !target(beforeinstall)
beforeinstall:
.endif

realinstall:
#	ranlib lib${LIB}.a
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m 600 lib${LIB}.a \
	    ${DESTDIR}${LIBDIR}
	${RANLIB} -t ${DESTDIR}${LIBDIR}/lib${LIB}.a
	chmod ${LIBMODE} ${DESTDIR}${LIBDIR}/lib${LIB}.a
.if !defined(NOPROFILE)
#	ranlib lib${LIB}_p.a
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m 600 \
	    lib${LIB}_p.a ${DESTDIR}${LIBDIR}
	${RANLIB} -t ${DESTDIR}${LIBDIR}/lib${LIB}_p.a
	chmod ${LIBMODE} ${DESTDIR}${LIBDIR}/lib${LIB}_p.a
.endif
.if !defined(NOPIC)
#	ranlib lib${LIB}_pic.a
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m 600 \
	    lib${LIB}_pic.a ${DESTDIR}${LIBDIR}
	${RANLIB} -t ${DESTDIR}${LIBDIR}/lib${LIB}_pic.a
	chmod ${LIBMODE} ${DESTDIR}${LIBDIR}/lib${LIB}_pic.a
.endif
.if !defined(NOPIC) && defined(SHLIB_MAJOR) && defined(SHLIB_MINOR)
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} ${DESTDIR}${LIBDIR}
.if (${SHLIB_TYPE} == "ELF")
	rm -f ${DESTDIR}${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}
	ln -s lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	    ${DESTDIR}${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}
	rm -f ${DESTDIR}${LIBDIR}/lib${LIB}.so
	ln -s lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	    ${DESTDIR}${LIBDIR}/lib${LIB}.so
.endif
.endif
.if !defined(NOLINT)
	${INSTALL} ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    llib-l${LIB}.ln ${DESTDIR}${LINTLIBDIR}
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
.if defined(SYMLINKS) && !empty(SYMLINKS)
	@set ${SYMLINKS}; \
	while test $$# -ge 2; do \
		l=$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo $$t -\> $$l; \
		rm -f $$t; \
		ln -s $$l $$t; \
	done; true
.endif

install: maninstall _SUBDIRUSE
maninstall: afterinstall
afterinstall: realinstall
realinstall: beforeinstall
.endif

.if !defined(NOMAN)
.include <bsd.man.mk>
.endif

.if !defined(NONLS)
.include <bsd.nls.mk>
.endif

.include <bsd.obj.mk>
.include <bsd.dep.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
