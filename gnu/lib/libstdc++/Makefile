#	$NetBSD: Makefile,v 1.1.1.1 1998/03/29 11:40:13 mrg Exp $

INCS=	cassert cerrno cfloat ciso646 climits clocale cmath complex csetjmp \
	csignal cstdarg cstddef cstdio cstdlib cstring ctime cwchar cwctype \
	fstream iomanip iosfwd stdexcept string strstream
INCS+=	stl.h complex.h _G_config.h
INCSDIR=/usr/include/g++

SUBDIR=std stl io

LIB=	stdc++

SRCS=	PlotFile.cc SFile.cc builtinbuf.cc editbuf.cc filebuf.cc fstream.cc \
indstream.cc ioassign.cc ioextend.cc iomanip.cc iostream.cc isgetline.cc \
isgetsb.cc isscan.cc osform.cc parsestream.cc pfstream.cc procbuf.cc \
sbform.cc sbgetline.cc sbscan.cc stdiostream.cc stdstrbufs.cc stdstreams.cc \
stream.cc streambuf.cc strstream.cc cleanup.c filedoalloc.c fileops.c \
floatconv.c genops.c iofclose.c iofgetpos.c iofread.c iofscanf.c \
iofsetpos.c iogetdelim.c iogetline.c ioignore.c iopadn.c iopopen.c \
ioprims.c ioprintf.c ioseekoff.c ioseekpos.c iostrerror.c ioungetc.c \
iovfprintf.c iovfscanf.c outfloat.c strops.c cmathi.cc cstdlibi.cc \
cstringi.cc stdexcepti.cc \
cstrmain.cc cstrio.cc fcomplex.cc fcomio.cc dcomplex.cc dcomio.cc \
ldcomplex.cc ldcomio.cc 

DIST=		$(.CURDIR)/../../dist
CXXFLAGS+=	-nostdinc++ 
CPPFLAGS+=	-I$(.CURDIR) -I$(DIST)/libstdc++ -I$(DIST)/libio \
		-I$(DIST)/libstdc++/stl -I$(DIST)/gcc/cp/inc
NOMAN=		noman
NOLINT=		nolint (c++)

.PATH:	$(DIST)/libstdc++ $(DIST)/libstdc++/stl $(DIST)/libio

.if (${MACHINE} != "alpha")
LDADD=	${DESTDIR}/usr/lib/c++rt0.o
DPADD=	${DESTDIR}/usr/lib/c++rt0.o
.endif

.include <bsd.lib.mk>
.include <bsd.subdir.mk>

SINSTCC=	$(DIST)/libstdc++/sinst.cc
CINSTCC=	$(DIST)/libstdc++/cinst.cc

cstrmain.cc: Makefile
	@echo Creating cstrmain.cc
	@(for i in REP MAIN TRAITS ADDSS ADDPS ADDCS ADDSP ADDSC EQSS EQPS EQSP NESS NEPS NESP LTSS LTPS LTSP GTSS GTPS GTSP LESS LEPS LESP GESS GEPS GESP ; do \
		echo "#define $$i"; \
		done;\
	echo '#define C';\
	cat $(SINSTCC)) > $(.TARGET)
CLEANFILES+=cstrmain.cc

cstrio.cc: Makefile
	@echo Creating cstrio.cc
	@(for i in EXTRACT INSERT GETLINE ; do \
		echo "#define $$i"; \
		done;\
	echo '#define C';\
	cat $(SINSTCC)) > $(.TARGET)
CLEANFILES+=cstrio.cc

fcomplex.cc: Makefile
	@echo Creating fcomplex.cc
	@(for i in MAIN ADDCC ADDCF ADDFC SUBCC SUBCF SUBFC MULCC MULCF MULFC DIVCC DIVCF DIVFC PLUS MINUS EQCC EQCF EQFC NECC NECF NEFC ABS ARG POLAR CONJ NORM COS COSH EXP LOG POWCC POWCF POWCI POWFC SIN SINH SQRT ; do \
		echo "#define $$i"; \
		done;\
	echo '#define F';\
	cat $(CINSTCC)) > $(.TARGET)
CLEANFILES+=fcomplex.cc

fcomio.cc: Makefile
	@echo Creating fcomio.cc
	@(for i in EXTRACT INSERT ; do \
		echo "#define $$i"; \
		done;\
	echo '#define F';\
	cat $(CINSTCC)) > $(.TARGET)
CLEANFILES+=fcomio.cc

dcomplex.cc: Makefile
	@echo Creating dcomplex.cc
	@(for i in MAIN ADDCC ADDCF ADDFC SUBCC SUBCF SUBFC MULCC MULCF MULFC DIVCC DIVCF DIVFC PLUS MINUS EQCC EQCF EQFC NECC NECF NEFC ABS ARG POLAR CONJ NORM COS COSH EXP LOG POWCC POWCF POWCI POWFC SIN SINH SQRT ; do \
		echo "#define $$i"; \
		done;\
	echo '#define D';\
	cat $(CINSTCC)) > $(.TARGET)
CLEANFILES+=dcomplex.cc

dcomio.cc: Makefile
	@echo Creating dcomio.cc
	@(for i in EXTRACT INSERT ; do \
		echo "#define $$i"; \
		done;\
	echo '#define D';\
	cat $(CINSTCC)) > $(.TARGET)
CLEANFILES+=dcomio.cc

ldcomplex.cc: Makefile
	@echo Creating ldcomplex.cc
	@(for i in MAIN ADDCC ADDCF ADDFC SUBCC SUBCF SUBFC MULCC MULCF MULFC DIVCC DIVCF DIVFC PLUS MINUS EQCC EQCF EQFC NECC NECF NEFC ABS ARG POLAR CONJ NORM COS COSH EXP LOG POWCC POWCF POWCI POWFC SIN SINH SQRT ; do \
		echo "#define $$i"; \
		done;\
	echo '#define LD';\
	cat $(CINSTCC)) > $(.TARGET)
CLEANFILES+=ldcomplex.cc

ldcomio.cc: Makefile
	@echo Creating ldcomio.cc
	@(for i in EXTRACT INSERT ; do \
		echo "#define $$i"; \
		done;\
	echo '#define LD';\
	cat $(CINSTCC)) > $(.TARGET)
CLEANFILES+=ldcomio.cc

stdexcepti.o: stdexcepti.cc
	@echo "$(COMPILE.cc) -frtti $(.IMPSRC)"
	@$(COMPILE.cc) -frtti $(.IMPSRC) -o $(.TARGET).o
	@$(LD) -x -r $(.TARGET).o -o $(.TARGET)
	@rm -f $(.TARGET).o

stdexcepti.po: stdexcepti.cc
	@echo "$(COMPILE.cc) -p -frtti $(.IMPSRC) -o $(.TARGET)"
	@$(COMPILE.cc) -p -frtti $(.IMPSRC) -o $(.TARGET).o
	@$(LD) -X -r $(.TARGET).o -o $(.TARGET)
	@rm -f $(.TARGET).o

stdexcepti.so: stdexcepti.cc
	@echo "$(COMPILE.cc) $(CPICFLAGS) -frtti $(.IMPSRC) -o $(.TARGET)"
	@$(COMPILE.cc) $(CPICFLAGS) -frtti $(.IMPSRC) -o $(.TARGET).o
	@$(LD) -x -r $(.TARGET).o -o $(.TARGET)
	@rm -f $(.TARGET).o
