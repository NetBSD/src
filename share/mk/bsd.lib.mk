#	$NetBSD: bsd.lib.mk,v 1.149 1999/02/12 12:38:45 lukem Exp $
#	@(#)bsd.lib.mk	8.3 (Berkeley) 4/22/94

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.include <bsd.obj.mk>
.MAIN:		all
.endif

.PHONY:		checkver cleanlib libinstall
realinstall:	checkver libinstall
clean cleandir distclean: cleanlib

.if exists(${.CURDIR}/shlib_version)
SHLIB_MAJOR != . ${.CURDIR}/shlib_version ; echo $$major
SHLIB_MINOR != . ${.CURDIR}/shlib_version ; echo $$minor

# Check for higher installed library versions.
.if !defined(NOCHECKVER) && !defined(NOCHECKVER_${LIB}) && \
	exists(${BSDSRCDIR}/lib/checkver)
checkver:
	@(cd ${.CURDIR} && \
		${BSDSRCDIR}/lib/checkver -d ${DESTDIR}${LIBDIR} ${LIB})
.else
checkver:
.endif
.else
checkver:
.endif

# add additional suffixes not exported.
# .po is used for profiling object files.
# .so is used for PIC object files.
.SUFFIXES: .out .a .ln .so .po .o .s .S .c .cc .C .m .F .f .r .y .l .cl .p .h
.SUFFIXES: .sh .m4 .m


# Set PICFLAGS to cc flags for producing position-independent code,
# if not already set.  Includes -DPIC, if required.

# Data-driven table using make variables to control  how shared libraries
# are built for different platforms and object formats.
# OBJECT_FMT:		currently either "ELF" or "a.out", from <bsd.own.mk>
# SHLIB_SOVERSION:  	version number to be compiled into a shared library
#                    	via -soname. Usualy ${SHLIB_MAJOR} on ELF.
#   			NetBSD/pmax used to use ${SHLIB_MAJOR}.{SHLIB-MINOR}.
# SHLIB_SHFLAGS:	Flags to tell ${LD} to emit  shared library.
#			with ELF, also set shared-lib version for ld.so.
# SHLIB_LDSTARTFILE:	support .o file, call C++ file-level constructors
# SHLIB_LDENDFILE:	support .o file, call C++ file-level destructors
# CPPICFLAGS:	flags for ${CPP} to preprocess  .[sS]  files for ${AS}
# CPICFLAGS:	flags for ${CC} to compile  .[cC] files to .so objects.
# CAPICFLAGS	flags for {$CC} to compiling .[Ss] files
#		 	(usually just ${CPPPICFLAGS} ${CPICFLAGS})
# APICFLAGS:	flags for ${AS} to assemble .[sS]  to .so objects.

.if ${MACHINE_ARCH} == "alpha"
		# Alpha-specific shared library flags
CPICFLAGS ?= -fpic -DPIC
CPPPICFLAGS?= -DPIC 
CAPICFLAGS?= ${CPPPICFLAGS} ${CPICFLAGS}
APICFLAGS ?=
.elif ${MACHINE_ARCH} == "mips"
		# mips-specific shared library flags

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

# Platform-independent flags for NetBSD a.out shared libraries (and PowerPC)
SHLIB_LDSTARTFILE=
SHLIB_LDENDFILE=
SHLIB_SHFLAGS=
SHLIB_SOVERSION=${SHLIB_MAJOR}.${SHLIB_MINOR}
CPICFLAGS?= -fpic -DPIC
CPPPICFLAGS?= -DPIC 
CAPICFLAGS?= ${CPPPICFLAGS} ${CPICFLAGS}
APICFLAGS?= -k

.endif

# Platform-independent linker flags for ELF shared libraries
.if ${OBJECT_FMT} == "ELF"
SHLIB_SOVERSION=${SHLIB_MAJOR}
SHLIB_SHFLAGS=-soname lib${LIB}.so.${SHLIB_SOVERSION}
SHLIB_LDSTARTFILE= ${DESTDIR}/usr/lib/crtbeginS.o
SHLIB_LDENDFILE= ${DESTDIR}/usr/lib/crtendS.o
.endif

CFLAGS+=	${COPTS}

.c.o:
	@echo ${COMPILE.c:Q} ${.IMPSRC}
	@${COMPILE.c} ${.IMPSRC} -o ${.TARGET}.o
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
	${LINT} ${LINTFLAGS} ${CPPFLAGS:M-[IDU]*} -i ${.IMPSRC}

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

.m.o:
	@echo ${COMPILE.m:Q} ${.IMPSRC}
	@${COMPILE.m} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.m.po:
	@echo ${COMPILE.m:Q} -pg ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.m} -pg ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.m.so:
	@echo ${COMPILE.m:Q} ${CPICFLAGS} ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.m} ${CPICFLAGS} ${.IMPSRC} -o ${.TARGET}.o
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
	@echo ${COMPILE.S:Q} ${CAPICFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} -o ${.TARGET}
	@${COMPILE.S} ${CAPICFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.if ${MKPIC} == "no" || (defined(LDSTATIC) && ${LDSTATIC} != "") \
	|| ${MKLINKLIB} != "no"
_LIBS=lib${LIB}.a
.else
_LIBS=
.endif

.if ${MKPROFILE} != "no"
_LIBS+=lib${LIB}_p.a
.endif

.if ${MKPIC} != "no"
_LIBS+=lib${LIB}_pic.a
.if defined(SHLIB_MAJOR) && defined(SHLIB_MINOR)
_LIBS+=lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
.endif
.endif

.if ${MKLINT} != "no" && ${MKLINKLIB} != "no"
_LIBS+=llib-l${LIB}.ln
.endif

all: ${SRCS} ${_LIBS}

__archivebuild: .USE
	@rm -f ${.TARGET}
	@${AR} cq ${.TARGET} `NM=${NM} ${LORDER} ${.ALLSRC:M*o} | ${TSORT}`
	${RANLIB} ${.TARGET}

__archiveinstall: .USE
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o ${LIBOWN} -g ${LIBGRP} \
		-m 600 ${.ALLSRC} ${.TARGET}
	${RANLIB} -t ${.TARGET}
	chmod ${LIBMODE} ${.TARGET}

DPSRCS+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${DPSRCS}
.if defined(YHEADER)
CLEANFILES+=	${SRCS:M*.y:.y=.h}
.endif

OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.o/g}
lib${LIB}.a:: ${OBJS} __archivebuild
	@echo building standard ${LIB} library

POBJS+=		${OBJS:.o=.po}
lib${LIB}_p.a:: ${POBJS} __archivebuild
	@echo building profiled ${LIB} library

SOBJS+=		${OBJS:.o=.so}
lib${LIB}_pic.a:: ${SOBJS} __archivebuild
	@echo building shared object ${LIB} library

lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}: lib${LIB}_pic.a ${DPADD} \
    ${SHLIB_LDSTARTFILE} ${SHLIB_LDENDFILE}
	@echo building shared ${LIB} library \(version ${SHLIB_MAJOR}.${SHLIB_MINOR}\)
	@rm -f lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
	$(LD) -x -shared ${SHLIB_SHFLAGS} -o ${.TARGET} \
	    ${SHLIB_LDSTARTFILE} \
	    --whole-archive lib${LIB}_pic.a --no-whole-archive ${LDADD} \
	    ${SHLIB_LDENDFILE}
.if ${OBJECT_FMT} == "ELF"
	rm -f lib${LIB}.so.${SHLIB_MAJOR}
	ln -s lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	    lib${LIB}.so.${SHLIB_MAJOR}
	rm -f lib${LIB}.so
	ln -s lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	    lib${LIB}.so
.endif

LOBJS+=		${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
LLIBS?=		-lc
llib-l${LIB}.ln: ${LOBJS}
	@echo building llib-l${LIB}.ln
	@rm -f llib-l${LIB}.ln
	@${LINT} -C${LIB} ${.ALLSRC} ${LLIBS}

cleanlib:
	rm -f a.out [Ee]rrs mklog core *.core ${CLEANFILES}
	rm -f lib${LIB}.a ${OBJS}
	rm -f lib${LIB}_p.a ${POBJS}
	rm -f lib${LIB}_pic.a lib${LIB}.so.* lib${LIB}.so ${SOBJS}
	rm -f llib-l${LIB}.ln ${LOBJS}

.if defined(SRCS)
afterdepend: .depend
	@(TMP=/tmp/_depend$$$$; \
	    sed -e 's/^\([^\.]*\).o[ ]*:/\1.o \1.po \1.so \1.ln:/' \
	      < .depend > $$TMP; \
	    mv $$TMP .depend)
.endif

.if !target(libinstall)
# Make sure it gets defined, in case MKPIC==no && MKLINKLIB==no
libinstall::

.if ${MKLINKLIB} != "no"
libinstall:: ${DESTDIR}${LIBDIR}/lib${LIB}.a
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${LIBDIR}/lib${LIB}.a
.endif
.if !defined(BUILD)
${DESTDIR}${LIBDIR}/lib${LIB}.a: .MADE
.endif

.PRECIOUS: ${DESTDIR}${LIBDIR}/lib${LIB}.a
${DESTDIR}${LIBDIR}/lib${LIB}.a: lib${LIB}.a __archiveinstall
.endif

.if ${MKPROFILE} != "no"
libinstall:: ${DESTDIR}${LIBDIR}/lib${LIB}_p.a
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${LIBDIR}/lib${LIB}_p.a
.endif
.if !defined(BUILD)
${DESTDIR}${LIBDIR}/lib${LIB}_p.a: .MADE
.endif

.PRECIOUS: ${DESTDIR}${LIBDIR}/lib${LIB}_p.a
${DESTDIR}${LIBDIR}/lib${LIB}_p.a: lib${LIB}_p.a __archiveinstall
.endif

.if ${MKPIC} != "no" && ${MKPICINSTALL} != "no"
libinstall:: ${DESTDIR}${LIBDIR}/lib${LIB}_pic.a
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${LIBDIR}/lib${LIB}_pic.a
.endif
.if !defined(BUILD)
${DESTDIR}${LIBDIR}/lib${LIB}_pic.a: .MADE
.endif

.PRECIOUS: ${DESTDIR}${LIBDIR}/lib${LIB}_pic.a
${DESTDIR}${LIBDIR}/lib${LIB}_pic.a: lib${LIB}_pic.a __archiveinstall
.endif

.if ${MKPIC} != "no" && defined(SHLIB_MAJOR) && defined(SHLIB_MINOR)
libinstall:: ${DESTDIR}${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
.endif
.if !defined(BUILD)
${DESTDIR}${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
${DESTDIR}${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}: lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o ${LIBOWN} -g ${LIBGRP} \
		-m ${LIBMODE} ${.ALLSRC} ${.TARGET}
.if ${OBJECT_FMT} == "a.out" && !defined(DESTDIR)
	/sbin/ldconfig -m ${LIBDIR}
.endif
.if ${OBJECT_FMT} == "ELF"
	rm -f ${DESTDIR}${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}
	ln -s lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	    ${DESTDIR}${LIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}
	rm -f ${DESTDIR}${LIBDIR}/lib${LIB}.so
.if ${MKLINKLIB} != "no"
	ln -s lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	    ${DESTDIR}${LIBDIR}/lib${LIB}.so
.endif
.endif
.endif

.if ${MKLINT} != "no" && ${MKLINKLIB} != "no"
libinstall:: ${DESTDIR}${LINTLIBDIR}/llib-l${LIB}.ln
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${LINTLIBDIR}/llib-l${LIB}.ln
.endif
.if !defined(BUILD)
${DESTDIR}${LINTLIBDIR}/llib-l${LIB}.ln: .MADE
.endif

.PRECIOUS: ${DESTDIR}${LINTLIBDIR}/llib-l${LIB}.ln
${DESTDIR}${LINTLIBDIR}/llib-l${LIB}.ln: llib-l${LIB}.ln
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o ${LIBOWN} -g ${LIBGRP} \
		-m ${LIBMODE} ${.ALLSRC} ${DESTDIR}${LINTLIBDIR}
.endif
.endif

.include <bsd.man.mk>
.include <bsd.nls.mk>
.include <bsd.files.mk>
.include <bsd.inc.mk>
.include <bsd.links.mk>
.include <bsd.dep.mk>
.include <bsd.sys.mk>

# Make sure all of the standard targets are defined, even if they do nothing.
lint regress:
