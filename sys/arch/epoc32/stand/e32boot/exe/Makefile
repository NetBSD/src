#	$NetBSD: Makefile,v 1.3.4.4 2017/12/03 11:36:02 jdolecek Exp $

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.inc.mk>

PROG=e32boot.exe

SRCS=e32boot.cpp netbsd.cpp
OBJS=$(SRCS:R:S/$/.o/g)

IN=$(PROG:.exe=.in)
MAP=$(PROG:.exe=.map)

NOMAN=		#defined
BINMODE= 444

S=../../../../../

CPPFLAGS=-c -nostdinc -Wall -Wno-ctor-dtor-privacy -O
CPPFLAGS+=-mcpu=arm710
CPPFLAGS+=-mapcs-32 -mshort-load-bytes -msoft-float -fcheck-new -fvtable-thunks
CPPFLAGS+=-D__SYMBIAN32__ -D__PSISOFT32__ -D__GCC32__ -D__EPOC32__ -D__MARM__
CPPFLAGS+=-U_UNICODE
CPPFLAGS+=-I. -I../include/ -I-
CPPFLAGS+=-I$(EPOC32_SDKDIR)/include/

CFLAGS=-ffreestanding
_CXXSEED=	# XXXXX

HEAPMIN=0x80000
HEAPMAX=0x8000000

EPOC32_LIBRARY_PATH=$(EPOC32_SDKDIR)/Release/MARM/Rel/
LIBS=$(EPOC32_LIBRARY_PATH)/EFSRV.LIB
LIBS+=$(EPOC32_LIBRARY_PATH)/EUSER.LIB
LIBS+=$(EPOC32_LIBRARY_PATH)/WS32.LIB


all: check_epoc32_cross $(PROG)

$(PROG): $(OBJS) $(IN)
	${_MKTARGET_LINK}
	$(LD) -s --base-file $(PROG:.exe=.bas) \
	    -o $@ --whole-archive $(IN) --no-whole-archive $(LIBS)
	TMP=. $(EXTERNAL_TOOLCHAIN)/bin/$(MACHINE_GNU_PLATFORM)-dlltool \
	    -S $(AS) \
	    --output-exp $(PROG:.exe=.exp) --base-file $(PROG:.exe=.bas) \
	    $(IN) $(LIBS)
	$(LD) -s -Map $(MAP) -o $@ $(PROG:.exe=.exp) \
	    --whole-archive $(IN) --no-whole-archive $(LIBS)
	mv $@ $@.tmp
	$(PETRAN) $@.tmp $@ -heap $(HEAPMIN) $(HEAPMAX) \
	    -nocall -uid1 0x1000007a -uid2 0x00000000 -uid3 0x00000000
	rm $@.tmp $(PROG:.exe=.bas) $(PROG:.exe=.exp)

$(IN): $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@

clean:
	rm -f $(PROG) $(OBJS) $(IN) $(MAP)
	rm -f $(PROG:.exe=.bas) $(PROG:.exe=.exp)

.include "${S}/conf/newvers_stand.mk"

.include <bsd.files.mk>
