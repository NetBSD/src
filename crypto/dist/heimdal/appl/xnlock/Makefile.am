# $Heimdal: Makefile.am,v 1.15.12.1 2003/05/06 16:50:33 joda Exp $
# $NetBSD: Makefile.am,v 1.1.1.5 2003/05/15 20:28:43 lha Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_krb4) $(X_CFLAGS)

WFLAGS += $(WFLAGS_NOIMPLICITINT)

if HAVE_X

bin_PROGRAMS = xnlock

else

bin_PROGRAMS =

endif

CHECK_LOCAL = 

man_MANS = xnlock.1

EXTRA_DIST = $(man_MANS) nose.0.left nose.0.right nose.1.left nose.1.right \
	nose.down nose.front nose.left.front nose.right.front

LDADD = \
	$(LIB_kafs) \
	$(LIB_krb5) \
	$(LIB_krb4) \
	$(LIB_des) \
	$(LIB_roken) \
	$(X_LIBS) -lXt $(X_PRE_LIBS) -lX11 $(X_EXTRA_LIBS)
