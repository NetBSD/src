# $Heimdal: Makefile.am 18468 2006-10-14 13:50:51Z lha $
# $NetBSD: Makefile.am,v 1.1 2008/03/22 09:29:55 mlelstv Exp $

include $(top_srcdir)/Makefile.am.common

noinst_PROGRAMS = gssmask gssmaestro

gssmask_SOURCES = gssmask.c common.c common.h protocol.h

gssmaestro_SOURCES = gssmaestro.c common.c common.h protocol.h

LDADD = $(top_builddir)/lib/gssapi/libgssapi.la $(LIB_roken)

