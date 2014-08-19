#	$NetBSD: Makefile,v 1.3.4.3 2014/08/20 00:02:52 tls Exp $

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.inc.mk>

PROG=e32boot.ldd

SRCS=e32boot.cpp epoc32.cpp cpu.cpp
OBJS=$(SRCS:R:S/$/.o/g)

IN=$(PROG:.ldd=.in)
MAP=$(PROG:.ldd=.map)

NOMAN=		#defined
BINMODE= 444

S=../../../../../

CPPFLAGS=-c -nostdinc -Wall -Wno-ctor-dtor-privacy -O
CPPFLAGS+=-mcpu=arm710
CPPFLAGS+=-mapcs-32 -mshort-load-bytes -msoft-float -fcheck-new -fvtable-thunks
CPPFLAGS+=-D__SYMBIAN32__ -D__PSISOFT32__ -D__GCC32__ -D__EPOC32__ -D__MARM__
CPPFLAGS+=-U_UNICODE
CPPFLAGS+=-I. -I../include/ -I- -I$(EPOC32_SDKDIR)/include/

CPPFLAGS+=-D_LOCORE -D__ASSEMBLER__	# XXXXXX

CFLAGS=
_CXXSEED=	# XXXXX

EPOC32_LIBRARY_PATH=$(EPOC32_SDKDIR)/Release/MARM/Rel/
LIBS=$(EPOC32_LIBRARY_PATH)/EKERN.LIB $(EPOC32_LIBRARY_PATH)/EUSER.LIB

S5_EKERNID=	100000b9
S5_UID2=	0x100000ac

S5MX_EKERNID=	100000ba
S5MX_UID2=	0x100000ae

#S7MX_EKERNID=	100000??
#S7MX_UID2=	0x100000??


all: check_epoc32_cross arm $(PROG:.ldd=-s5.ldd) $(PROG:.ldd=-s5mx.ldd)

arm:
	ln -s ../../../../$@/include $@

$(PROG:.ldd=-s5.ldd): $(PROG)
	$(EXTERNAL_TOOLCHAIN)/bin/petran $(PROG) $@.tmp \
	    -nocall -uid1 0x10000079 -uid2 $(S5_UID2) -uid3 0x00000000
	sed -e "s/\(EKERN\[\)[0-9a-f]\{8\}\(\]\.EXE\)/\1$(S5_EKERNID)\2/" \
	    $@.tmp > $@
	rm $@.tmp

$(PROG:.ldd=-s5mx.ldd): $(PROG)
	$(EXTERNAL_TOOLCHAIN)/bin/petran $(PROG) $@.tmp \
	    -nocall -uid1 0x10000079 -uid2 $(S5MX_UID2) -uid3 0x00000000
	sed -e "s/\(EKERN\[\)[0-9a-f]\{8\}\(\]\.EXE\)/\1$(S5MX_EKERNID)\2/" \
	    $@.tmp > $@
	rm $@.tmp

$(PROG): $(OBJS) $(IN)
	$(LD) -s --base-file $(PROG:.ldd=.bas) \
	    -o $@ --whole-archive $(IN) --no-whole-archive $(LIBS)
	TMP=. $(EXTERNAL_TOOLCHAIN)/bin/$(MACHINE_GNU_PLATFORM)-dlltool \
	    -S $(AS) \
	    --output-exp $(PROG:.ldd=.exp) --base-file $(PROG:.ldd=.bas) \
	    $(IN) $(LIBS)
	$(LD) -s -Map $(MAP) -o $@ $(PROG:.ldd=.exp) \
	    --whole-archive $(IN) --no-whole-archive $(LIBS)
	rm $(PROG:.ldd=.bas) $(PROG:.ldd=.exp)

$(IN): $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@


clean:
	rm -f $(PROG) $(OBJS) $(IN) $(MAP)
	rm -f $(PROG:.ldd=-s5.ldd) $(PROG:.ldd=-s5mx.ldd)
	rm -f $(PROG:.ldd=.bas) $(PROG:.ldd=.exp)
	rm -f arm

.include <bsd.files.mk>
