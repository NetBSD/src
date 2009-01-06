#	$NetBSD: Makefile,v 1.1 2009/01/06 03:59:56 mrg Exp $

.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "sparc64") || \
    (${MACHINE_ARCH} == "x86_64") || (${MACHINE_ARCH} == "powerpc64")

CPPFLAGS+=	-DELFSIZE=64
LIB=	ldd_elf64

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

.include "../Makefile.elf"

.endif
