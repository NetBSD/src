#	$NetBSD: Makefile.inc,v 1.21 2018/04/03 21:10:27 joerg Exp $

SRCS+=		rtld_start.S mdreloc.c
#CPUFLAGS.rtld_start.S+=	-marm

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
CPPFLAGS+=	-DRTLD_COMMON_CALL_IFUNC_REL
.if !empty(LDELFSO_MACHINE_ARCH:Mearm*)
CPPFLAGS+=	-DHAVE_INITFINI_ARRAY
CPPFLAGS+=	-DELF_NOTE_MARCH_DESC=\"${LDELFSO_MACHINE_ARCH}\"
SRCS+=		find_exidx.c
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
