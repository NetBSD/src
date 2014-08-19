#	$NetBSD: Makefile,v 1.2.2.1 2014/08/20 00:02:20 tls Exp $

.include <bsd.own.mk>

WARNS?=5
USE_SHLIBDIR=	yes

PPATHLIBDIR=${NETBSDSRCDIR}/common/lib/libppath

.include "${PPATHLIBDIR}/Makefile.inc"

#CPPFLAGS+=-D_LIBPPATH
#CPPFLAGS+=-D_REENTRANT -I${NETBSDSRCDIR}/lib/libc/include

SRCS+=ppath_malloc.c

LIBDPLIBS+=	prop		${NETBSDSRCDIR}/lib/libprop

LINTFLAGS+=-w
LIB=	ppath

MAN+=		ppath.3
MLINKS+=	ppath.3 ppath_component_at.3
MLINKS+=	ppath.3 ppath_component_idx.3
MLINKS+=	ppath.3 ppath_component_key.3
MLINKS+=	ppath.3 ppath_component_release.3
MLINKS+=	ppath.3 ppath_component_retain.3
MLINKS+=	ppath.3 ppath_copy.3
MLINKS+=	ppath.3 ppath_create.3
MLINKS+=	ppath.3 ppath_idx.3
MLINKS+=	ppath.3 ppath_key.3
MLINKS+=	ppath.3 ppath_length.3
MLINKS+=	ppath.3 ppath_lookup.3
MLINKS+=	ppath.3 ppath_pop.3
MLINKS+=	ppath.3 ppath_push.3
MLINKS+=	ppath.3 ppath_push_idx.3
MLINKS+=	ppath.3 ppath_push_key.3
MLINKS+=	ppath.3 ppath_release.3
MLINKS+=	ppath.3 ppath_replace_idx.3
MLINKS+=	ppath.3 ppath_replace_key.3
MLINKS+=	ppath.3 ppath_retain.3
MLINKS+=	ppath.3 ppath_subpath.3

MAN+=		ppath_bool.3
MLINKS+=	ppath_bool.3 ppath_copydel_bool.3
MLINKS+=	ppath_bool.3 ppath_copyset_bool.3
MLINKS+=	ppath_bool.3 ppath_delete_bool.3
MLINKS+=	ppath_bool.3 ppath_get_bool.3
MLINKS+=	ppath_bool.3 ppath_set_bool.3

MAN+=		ppath_number.3
MLINKS+=	ppath_number.3 ppath_copydel_int64.3
MLINKS+=	ppath_number.3 ppath_copydel_uint64.3
MLINKS+=	ppath_number.3 ppath_copyset_int64.3
MLINKS+=	ppath_number.3 ppath_copyset_uint64.3
MLINKS+=	ppath_number.3 ppath_delete_int64.3
MLINKS+=	ppath_number.3 ppath_delete_uint64.3
MLINKS+=	ppath_number.3 ppath_get_int64.3
MLINKS+=	ppath_number.3 ppath_get_uint64.3
MLINKS+=	ppath_number.3 ppath_set_int64.3
MLINKS+=	ppath_number.3 ppath_set_uint64.3

MAN+=		ppath_object.3
MLINKS+=	ppath_object.3 ppath_copydel_object.3
MLINKS+=	ppath_object.3 ppath_copyset_object.3
MLINKS+=	ppath_object.3 ppath_delete_object.3
MLINKS+=	ppath_object.3 ppath_get_object.3
MLINKS+=	ppath_object.3 ppath_set_object.3

.include <bsd.lib.mk>
