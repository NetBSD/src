# $Heimdal: Makefile.am,v 1.2 2001/01/28 22:50:35 assar Exp $
# $NetBSD: Makefile.am,v 1.1.1.2 2002/09/12 12:41:33 joda Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_krb4)

bin_PROGRAMS = rcp

rcp_SOURCES  = rcp.c util.c

LDADD = $(LIB_roken)
