#	$NetBSD: Makefile.inc,v 1.1.104.1 2026/11/04 23:19:25 perseant Exp $

# DCE 1.1 compatible UUID implementation sources

.PATH: ${.PARSEDIR}

SRCS+=	uuid_compare.c uuid_create.c uuid_create_nil.c uuid_equal.c \
	uuid_from_string.c uuid_hash.c uuid_is_nil.c uuid_stream.c \
	uuid_to_string.c

MAN+=	uuid.3

MLINKS+=uuid.3 uuid_compare.3
MLINKS+=uuid.3 uuid_create.3
MLINKS+=uuid.3 uuid_create_nil.3
MLINKS+=uuid.3 uuid_equal.3
MLINKS+=uuid.3 uuid_from_string.3
MLINKS+=uuid.3 uuid_hash.3
MLINKS+=uuid.3 uuid_is_nil.3
MLINKS+=uuid.3 uuid_to_string.3

MLINKS+=uuid.3 uuid_enc_le.3
MLINKS+=uuid.3 uuid_dec_le.3
MLINKS+=uuid.3 uuid_enc_be.3
MLINKS+=uuid.3 uuid_dec_be.3
