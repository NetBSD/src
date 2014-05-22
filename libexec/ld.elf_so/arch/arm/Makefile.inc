#	$NetBSD: Makefile.inc,v 1.11.42.2 2014/05/22 11:37:13 yamt Exp $

SRCS+=		rtld_start.S mdreloc.c
#CPUFLAGS.rtld_start.S+=	-marm

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
.if !empty(LDELFSO_MACHINE_ARCH:Mearm*)
CPPFLAGS+=	-DHAVE_INITFINI_ARRAY
CPPFLAGS+=	-DELF_NOTE_MARCH_DESC=\"${LDELFSO_MACHINE_ARCH}\"
SRCS+=		find_exidx.c
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
