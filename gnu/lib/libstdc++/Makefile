#	$NetBSD: Makefile,v 1.8 1999/02/11 17:30:46 tv Exp $

LIB=		stdc++

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

NOMAN=
NOLINT=		nolint (c++)

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

.if (${OBJECT_FMT} == "a.out")
LDADD=	${DESTDIR}/usr/lib/c++rt0.o
DPADD=	${DESTDIR}/usr/lib/c++rt0.o
.endif
