AUTOMAKE_OPTIONS=	

bindir=		${exec_prefix}/${BINSUBDIR}
bin_PROGRAMS=	@MAKE_ADJTIMED@
BUILT_SOURCES=
CLEANFILES=
EXTRA_PROGRAMS=	adjtimed
AM_CPPFLAGS=	-I$(top_srcdir)/include
LDADD=		../libntp/libntp.a
ETAGS_ARGS=	Makefile.am

include $(top_srcdir)/bincheck.mf
include $(top_srcdir)/depsver.mf
