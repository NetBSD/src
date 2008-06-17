#	$NetBSD: Makefile.inc,v 1.6.8.1 2008/06/17 09:13:31 yamt Exp $

.PATH:	${.PARSEDIR}

SRCS+=	prop_array.c prop_array_util.c prop_bool.c prop_data.c \
        prop_dictionary.c prop_dictionary_util.c prop_ingest.c \
        prop_kern.c prop_number.c prop_object.c prop_stack.c prop_string.c

SRCS+=	prop_rb.c
