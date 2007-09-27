#	$NetBSD: Makefile.inc,v 1.5.4.1 2007/09/27 16:16:27 xtraeme Exp $

.PATH:	${.PARSEDIR}

SRCS+=	prop_array.c prop_bool.c prop_data.c prop_dictionary.c \
	prop_dictionary_util.c prop_ingest.c prop_kern.c prop_number.c \
	prop_object.c prop_stack.c prop_string.c

SRCS+=	prop_rb.c
