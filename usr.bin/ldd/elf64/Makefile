#	$NetBSD: Makefile,v 1.4.2.2.4.3 2009/12/23 03:14:47 mrg Exp $

CPPFLAGS+=	-DELFSIZE=64
LIB=	ldd_elf64

# XXX Force one member
SRCS=	dummy.c

LIBISPRIVATE=	yes
.PATH: ${.CURDIR}/..

.if ${MACHINE_ARCH:Mmips64*} != ""
ABI64:= ${CPUFLAGS:M-mabi=64:M-mabi=o64}
.endif

.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "sparc64") || \
    (${MACHINE_ARCH} == "x86_64") || (${MACHINE_ARCH} == "powerpc64") || \
    (${MACHINE_ARCH} == "mips64el") || (${MACHINE_ARCH} == "mips64eb")

# XXX we need to make sure that we don't accidentally get the elf32
# XXX versions of these.

RTLD_FUNCS	= \
	_rtld_expand_path \
	_rtld_digest_dynamic \
	_rtld_digest_phdr \
	_rtld_load_needed_objects \
	_rtld_load_object \
	_rtld_map_object \
	_rtld_obj_free \
	_rtld_obj_new \
	_rtld_add_paths \
	_rtld_process_hints \
	_rtld_sysctl \
	_rtld_load_library

.for _d in ${RTLD_FUNCS}
CPPFLAGS+=	-D${_d}=_elf64_${_d}
.endfor

.if (${MACHINE_ARCH} == "mips64el") || (${MACHINE_ARCH} == "mips64eb")
CPPFLAGS+=	-DRTLD_ARCH_SUBDIR=\"${MLIBDIR}\"
.endif

.include "../Makefile.elf"

.endif

.include <bsd.lib.mk>
