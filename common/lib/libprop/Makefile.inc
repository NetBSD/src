#	$NetBSD: Makefile.inc,v 1.6.10.1 2008/06/23 04:26:45 wrstuden Exp $

.PATH:	${.PARSEDIR}

SRCS+=	prop_array.c prop_array_util.c prop_bool.c prop_data.c \
        prop_dictionary.c prop_dictionary_util.c prop_ingest.c \
        prop_kern.c prop_number.c prop_object.c prop_stack.c prop_string.c

SRCS+=	prop_rb.c
