#	$NetBSD: Makefile,v 1.18 2002/02/28 02:13:34 tv Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		stdc++

.if defined(USE_NEW_TOOLCHAIN)

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	4
SHLIB_MINOR=	0

SUBDIR=		include io config

TOP=		${.CURDIR}/../..
DIST=		${TOP}/dist/toolchain

SRCS=		${G_OBJS:.o=.cc} ${G_LIBIBERTY_OBJS:.o=.c}
INCS=		_G_config.h
INCSDIR=	/usr/include/g++

# Ugly, but they aren't identified individually in libio/Makefile:
.for f in ${G_LIBIOSTREAM_USE}
.if exists(${DIST}/libio/${f:.o=.c})
SRCS+=		${f:.o=.c}
.else
SRCS+=		${f:.o=.cc}
.endif
.endfor

GCPPFLAGS=	${G_XCFLAGS} ${G_CINCLUDES} ${G_MT_CFLAGS} ${G_CXXINCLUDES}
CPPFLAGS+=	-nostdinc++ -I${.CURDIR}/arch/${MACHINE_ARCH} \
		${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*} \
		-I${DIST}/gcc/cp/inc \
		-I${.CURDIR}/../libiberty -I${DIST}/include

CXXFLAGS+=	${G_XCXXFLAGS:M-f*}

TEXINFO=	iostream.texi

.PATH: ${DIST}/libstdc++ ${DIST}/libstdc++/stl ${DIST}/libio ${DIST}/libiberty

.for t in C
.for v in STRFUNCS STRIO
CLEANFILES+=	${t}${v}.cc
SRCS+=		${t}${v}.cc

${t}${v}.cc: Makefile ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk sinst.cc
	echo '#define ${t}' >$@
.for f in ${G_${v}}
	echo '#define ${f}' >>$@
.endfor
	cat ${>:M*.cc} >>$@
.endfor
.endfor

.for t in F D LD
.for v in COMFUNCS COMIO
CLEANFILES+=	${t}${v}.cc
SRCS+=		${t}${v}.cc

${t}${v}.cc: Makefile ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk cinst.cc
	echo '#define ${t}' >$@
.for f in ${G_${v}}
	echo '#define ${f}' >>$@
.endfor
	cat ${>:M*.cc} >>$@
.endfor
.endfor

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
.include <bsd.info.mk>

${OBJS}: ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk
.else
.include <bsd.prog.mk> # do nothing
.endif

.else
#
# XXX - BEGIN - Old toolchain build goop - delete when everything migrated - XXX
#

SHLIB_MAJOR=	2
SHLIB_MINOR=	0

INCS=		cassert cctype cerrno cfloat ciso646 climits clocale cmath \
		complex csetjmp csignal cstdarg cstddef cstdio cstdlib \
		cstring ctime cwchar cwctype fstream iomanip iosfwd iostream \
		stdexcept string strstream complex.h stl.h _G_config.h
INCSDIR=	/usr/include/g++

SUBDIR=		config std stl io

SRCS=	PlotFile.cc SFile.cc builtinbuf.cc editbuf.cc filebuf.cc fstream.cc \
	indstream.cc ioassign.cc ioextend.cc iomanip.cc iostream.cc \
	isgetline.cc isgetsb.cc isscan.cc osform.cc parsestream.cc \
	pfstream.cc procbuf.cc sbform.cc sbgetline.cc sbscan.cc \
	stdiostream.cc stdstrbufs.cc stdstreams.cc stream.cc streambuf.cc \
	strstream.cc \
	cleanup.c filedoalloc.c fileops.c floatconv.c genops.c iofclose.c \
	iofdopen.c iofeof.c ioferror.c iofflush.c iofflush_u.c iofgetpos.c \
	iofgets.c iofopen.c iofprintf.c iofputs.c iofread.c iofscanf.c \
	iofsetpos.c ioftell.c iofwrite.c iogetc.c iogetdelim.c iogetline.c \
	iogets.c ioignore.c iopadn.c ioperror.c iopopen.c ioprims.c \
	ioprintf.c ioputc.c ioputs.c ioscanf.c ioseekoff.c ioseekpos.c \
	iosetbuffer.c iosetvbuf.c iosprintf.c iosscanf.c iostrerror.c \
	ioungetc.c iovfprintf.c iovfscanf.c iovsprintf.c iovsscanf.c \
	outfloat.c peekc.c strops.c \
	cmathi.cc cstdlibi.cc cstringi.cc stdexcepti.cc stlinst.cc

DIST=		${.CURDIR}/../../dist
CPPFLAGS+=	-I${DESTDIR}/usr/include/g++ -I${DIST}/libstdc++

TEXINFO=	iostream.texi

.PATH:	${DIST}/libstdc++ ${DIST}/libstdc++/stl ${DIST}/libio

# W not yet supported
.for _TYPE_ in C
SRCS+=		${_TYPE_}inst.cc
CLEANFILES+=	${_TYPE_}inst.cc
${_TYPE_}inst.cc: sinst.cc
	echo '#define ${_TYPE_}' >$@
	@for func in \
		REP MAIN TRAITS ADDSS ADDPS ADDCS ADDSP ADDSC EQSS EQPS EQSP \
		NESS NEPS NESP LTSS LTPS LTSP GTSS GTPS GTSP LESS LEPS LESP \
		GESS GEPS GESP EXTRACT INSERT GETLINE; do \
			echo "#define $$func" >>$@; \
	done
	cat $> >>$@
.endfor

.for _TYPE_ in F D LD
SRCS+=		${_TYPE_}inst.cc
CLEANFILES+=	${_TYPE_}inst.cc
${_TYPE_}inst.cc: cinst.cc
	echo '#define ${_TYPE_}' >$@
	@for func in \
		MAIN ADDCC ADDCF ADDFC SUBCC SUBCF SUBFC MULCC MULCF MULFC \
		DIVCC DIVCF DIVFC PLUS MINUS EQCC EQCF EQFC NECC NECF NEFC \
		ABS ARG POLAR CONJ NORM COS COSH EXP LOG POWCC POWCF POWCI \
		POWFC SIN SINH SQRT EXTRACT INSERT; do \
			echo "#define $$func" >>$@; \
	done
	cat $> >>$@
.endfor

.cc:
	@true

.include <bsd.info.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
#
# XXX - END - Old toolchain build goop - delete when everything migrated - XXX
#
.endif

.if (${OBJECT_FMT} == "a.out")
LDADD=	${DESTDIR}/usr/lib/c++rt0.o
DPADD=	${DESTDIR}/usr/lib/c++rt0.o
.endif
