# -*-Makefile-*-, or close enough

AUTOMAKE_OPTIONS =	1.7 foreign

# Only build the library when the system doesn't already have GNU glob.
if USE_LOCAL_GLOB
  noinst_LIBRARIES =	libglob.a
endif

libglob_a_SOURCES =	glob.c glob.h fnmatch.c fnmatch.h


EXTRA_DIST =		COPYING.LIB Makefile.ami SCOPTIONS SMakefile \
			configure.bat
