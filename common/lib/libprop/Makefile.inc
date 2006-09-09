#	$NetBSD: Makefile.inc,v 1.4 2006/09/09 06:59:28 thorpej Exp $

.PATH:	${.PARSEDIR}

SRCS+=	prop_array.c prop_bool.c prop_data.c prop_dictionary.c \
	prop_ingest.c prop_kern.c prop_number.c prop_object.c prop_string.c

SRCS+=	prop_rb.c
