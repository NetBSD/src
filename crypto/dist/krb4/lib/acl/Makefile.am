# $KTH-KRB: Makefile.am,v 1.6 2002/09/09 13:24:31 joda Exp $
# $NetBSD: Makefile.am,v 1.1.1.2 2002/09/12 12:22:08 joda Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += -DROKEN_RENAME $(INCLUDE_des)

include_HEADERS =  acl.h

build_HEADERZ = $(include_HEADERS)

lib_LTLIBRARIES = libacl.la
libacl_la_LDFLAGS = -version-info 1:0:0
libacl_la_LIBADD  = ../krb/libkrb.la

libacl_la_SOURCES = acl_files.c

man_MANS = acl_check.3

$(libacl_la_OBJECTS): roken_rename.h

roken_rename.h:
	$(LN_S) $(srcdir)/../krb/roken_rename.h .

