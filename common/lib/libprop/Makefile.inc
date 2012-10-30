#	$NetBSD: Makefile.inc,v 1.8.2.1 2012/10/30 18:46:15 yamt Exp $

.PATH:	${.PARSEDIR}

SRCS+=	prop_array.c prop_array_util.c prop_bool.c prop_data.c \
        prop_dictionary.c prop_dictionary_util.c prop_ingest.c \
        prop_kern.c prop_number.c prop_object.c prop_stack.c prop_string.c

.ifdef (PROPLIB_WANT_RB)
SRCS+=	prop_rb.c
.endif
