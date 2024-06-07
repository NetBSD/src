#	$NetBSD: Makefile.inc,v 1.2 2024/06/07 20:50:13 riastradh Exp $

# net sources
.PATH: ${.CURDIR}/nameser

SRCS+=	ns_name.c ns_netint.c ns_parse.c ns_print.c \
	ns_samedomain.c ns_ttl.c

# Table of char has entries initialized to -1.  It doesn't matter that
# the integer value that gets stored is 255 instead, even though the
# entries are added to a value of type int, because we won't use those
# table entries.
LINTFLAGS.ns_name.c+=	-X 221
