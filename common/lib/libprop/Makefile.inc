#	$NetBSD: Makefile.inc,v 1.10.16.2 2020/04/21 19:37:51 martin Exp $

.PATH:	${.PARSEDIR}

SRCS+=	prop_array.c prop_array_util.c prop_bool.c prop_data.c \
        prop_dictionary.c prop_dictionary_util.c prop_ingest.c \
        prop_kern.c prop_number.c prop_object.c prop_stack.c prop_string.c
