#	$NetBSD: Makefile,v 1.1 2008/05/22 13:57:47 lukem Exp $

.include "../../openldap.mk"

LIB=		lber

.PATH:		${LDAP_DISTDIR}/libraries/liblber
CPPFLAGS+=	-I${LDAP_DISTDIR}/libraries/liblber

SRCS=		assert.c decode.c encode.c io.c bprint.c debug.c \
		memory.c options.c sockbuf.c stdio.c

CPPFLAGS+=	-DLBER_LIBRARY

.PATH:		${LDAP_SRCDIR}/man

MAN=		lber-decode.3 lber-encode.3 lber-memory.3 lber-sockbuf.3 \
		lber-types.3

MLINKS+=	lber-decode.3 ber_get_next.3 \
		lber-decode.3 ber_skip_tag.3 \
		lber-decode.3 ber_peek_tag.3 \
		lber-decode.3 ber_scanf.3 \
		lber-decode.3 ber_get_int.3 \
		lber-decode.3 ber_get_stringa.3 \
		lber-decode.3 ber_get_stringb.3 \
		lber-decode.3 ber_get_null.3 \
		lber-decode.3 ber_get_enum.3 \
		lber-decode.3 ber_get_boolean.3 \
		lber-decode.3 ber_get_bitstring.3 \
		lber-decode.3 ber_first_element.3 \
		lber-decode.3 ber_next_element.3

MLINKS+=	lber-encode.3 ber_alloc_t.3 \
		lber-encode.3 ber_flush.3 \
		lber-encode.3 ber_printf.3 \
		lber-encode.3 ber_put_int.3 \
		lber-encode.3 ber_put_ostring.3 \
		lber-encode.3 ber_put_string.3 \
		lber-encode.3 ber_put_null.3 \
		lber-encode.3 ber_put_enum.3 \
		lber-encode.3 ber_start_set.3 \
		lber-encode.3 ber_put_seq.3 \
		lber-encode.3 ber_put_set.3

MLINKS+=	lber-types.3 ber_bvarray_add.3 \
		lber-types.3 ber_bvarray_free.3 \
		lber-types.3 ber_bvdup.3 \
		lber-types.3 ber_bvecadd.3 \
		lber-types.3 ber_bvecfree.3 \
		lber-types.3 ber_bvfree.3 \
		lber-types.3 ber_bvstr.3 \
		lber-types.3 ber_bvstrdup.3 \
		lber-types.3 ber_dupbv.3 \
		lber-types.3 ber_free.3 \
		lber-types.3 ber_str2bv.3

.include <bsd.lib.mk>
