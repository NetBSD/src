# $Id: Makefile.am,v 1.1.1.3 2001/09/17 12:09:49 assar Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_krb4) $(INCLUDE_hesiod)

bin_SCRIPTS		= pfrom

libexec_PROGRAMS	= push

push_SOURCES = push.c push_locl.h

pfrom: pfrom.in
	sed -e "s!%libexecdir%!$(libexecdir)!" $(srcdir)/pfrom.in > $@
	chmod +x $@

man_MANS = push.8 pfrom.1

CLEANFILES = pfrom

EXTRA_DIST = pfrom.in $(man_MANS)

LDADD = $(LIB_krb5) \
	$(LIB_krb4) \
	$(LIB_des) \
	$(LIB_roken) \
	$(LIB_hesiod)
