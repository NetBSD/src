# $KTH-KRB: Makefile.am,v 1.7 2002/09/09 19:17:04 joda Exp $
# $NetBSD: Makefile.am,v 1.1.1.2 2002/09/12 12:22:09 joda Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += -DROKEN_RENAME $(INCLUDE_des)

include_HEADERS =  kadm.h kadm_err.h

build_HEADERZ = $(include_HEADERS)

lib_LTLIBRARIES = libkadm.la
libkadm_la_LDFLAGS = -version-info 1:11:0
libkadm_la_LIBADD  = ../krb/libkrb.la $(LIB_des) $(LIB_com_err)

libkadm_la_SOURCES =				\
	kadm_cli_wrap.c				\
	kadm_err.c				\
	kadm_stream.c				\
	kadm_supp.c				\
	check_password.c


$(libkadm_la_OBJECTS): kadm_err.h roken_rename.h

roken_rename.h:
	$(LN_S) $(srcdir)/../krb/roken_rename.h .

kadm_err.h: kadm_err.et

CLEANFILES = kadm_err.h kadm_err.c
