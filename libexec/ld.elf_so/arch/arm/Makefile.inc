#	$NetBSD: Makefile.inc,v 1.21.2.1 2019/06/10 22:05:30 christos Exp $

SRCS+=		rtld_start.S mdreloc.c
#CPUFLAGS.rtld_start.S+=	-marm

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
CPPFLAGS+=	-DRTLD_COMMON_CALL_IFUNC_REL
.if !empty(LDELFSO_MACHINE_ARCH:Mearm*)
CPPFLAGS+=	-DELF_NOTE_MARCH_DESC=\"${LDELFSO_MACHINE_ARCH}\"
SRCS+=		find_exidx.c
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
