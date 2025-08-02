#	$NetBSD: Makefile.inc,v 1.10.36.1 2025/08/02 05:18:34 perseant Exp $

.PATH:	${.PARSEDIR}

SRCS+=	prop_array.c prop_array_util.c prop_bool.c prop_data.c \
        prop_dictionary.c prop_dictionary_util.c prop_extern.c prop_ingest.c \
        prop_intern.c prop_kern.c prop_number.c prop_object.c prop_stack.c \
	prop_string.c
