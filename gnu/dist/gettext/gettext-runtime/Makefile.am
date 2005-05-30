## Makefile for the gettext-runtime directory of GNU gettext
## Copyright (C) 1995-1999, 2000-2005 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.5 gnu no-dependencies
ACLOCAL_AMFLAGS = -I m4 -I ../gettext-tools/m4 -I ../autoconf-lib-link/m4 -I ../m4

MAKEINFO = env LANG= LC_MESSAGES= LC_ALL= LANGUAGE= @MAKEINFO@

SUBDIRS =      doc intl intl-java intl-csharp lib @SUBDIR_libasprintf@ src po m4
DIST_SUBDIRS = doc intl intl-java intl-csharp lib libasprintf          src po man m4

EXTRA_DIST = BUGS


# Files installed for the user and for use by gettextize.

gettextsrcdir = $(datadir)/gettext
gettextsrc_DATA = ABOUT-NLS


all-local: ABOUT-NLS

ABOUT-NLS: $(srcdir)/doc/nls.texi $(srcdir)/doc/matrix.texi
	rm -f $(srcdir)/ABOUT-NLS \
	  && $(MAKEINFO) --output=$(srcdir)/ABOUT-NLS --no-header \
	    -I $(srcdir)/doc --no-validate nls.texi


# VMS support.

config.h_vms: config.h.in ../version.sh
	(echo '#define KEEP_CRTL_SETLOCALE 1'; echo '#include <vms_jackets.h>'; echo '#undef realpath'; \
	 sed -e 's/#undef ENABLE_NLS$$/#define ENABLE_NLS 1/' \
	     -e 's/#undef HAVE_ALLOCA$$/#define HAVE_ALLOCA 1/' \
	     -e 's/#undef HAVE_DLFCN_H$$/#define HAVE_DLFCN_H 1/' \
	     -e 's/#undef HAVE_ERRNO_DECL$$/#define HAVE_ERRNO_DECL 1/' \
	     -e 's/#undef HAVE_GETCWD$$/#define HAVE_GETCWD 1/' \
	     -e 's/#undef HAVE_GETEGID$$/#define HAVE_GETEGID 1/' \
	     -e 's/#undef HAVE_GETEUID$$/#define HAVE_GETEUID 1/' \
	     -e 's/#undef HAVE_GETGID$$/#define HAVE_GETGID 1/' \
	     -e 's/#undef HAVE_GETPAGESIZE$$/#define HAVE_GETPAGESIZE 1/' \
	     -e 's/#undef HAVE_GETUID$$/#define HAVE_GETUID 1/' \
	     -e 's/#undef HAVE_ICONV$$/#define HAVE_ICONV 1/' \
	     -e 's/#undef HAVE_INTTYPES_H$$/#define HAVE_INTTYPES_H 1/' \
	     -e 's/#undef HAVE_LANGINFO_CODESET$$/#define HAVE_LANGINFO_CODESET 1/' \
	     -e 's/#undef HAVE_LC_MESSAGES$$/#define HAVE_LC_MESSAGES 1/' \
	     -e 's/#undef HAVE_LIMITS_H$$/#define HAVE_LIMITS_H 1/' \
	     -e 's/#undef HAVE_LOCALE_H$$/#define HAVE_LOCALE_H 1/' \
	     -e 's/#undef HAVE_LONG_DOUBLE$$/#define HAVE_LONG_DOUBLE 1/' \
	     -e 's/#undef HAVE_LONG_LONG$$/#define HAVE_LONG_LONG 1/' \
	     -e 's/#undef HAVE_MALLOC_H$$/#define HAVE_MALLOC_H 1/' \
	     -e 's/#undef HAVE_MEMMOVE$$/#define HAVE_MEMMOVE 1/' \
	     -e 's/#undef HAVE_MEMORY_H$$/#define HAVE_MEMORY_H 1/' \
	     -e 's/#undef HAVE_MMAP$$/#define HAVE_MMAP 1/' \
	     -e 's/#undef HAVE_MUNMAP$$/#define HAVE_MUNMAP 1/' \
	     -e 's/#undef HAVE_NL_TYPES_H$$/#define HAVE_NL_TYPES_H 1/' \
	     -e 's/#undef HAVE_POSIX_PRINTF$$/#define HAVE_POSIX_PRINTF 1/' \
	     -e 's/#undef HAVE_PTRDIFF_T$$/#define HAVE_PTRDIFF_T 1/' \
	     -e 's/#undef HAVE_PUTENV$$/#define HAVE_PUTENV 1/' \
	     -e 's/#undef HAVE_READLINK$$/#define HAVE_READLINK 1/' \
	     -e 's/#undef HAVE_SETENV$$/#define HAVE_SETENV 1/' \
	     -e 's/#undef HAVE_SETLOCALE$$/#define HAVE_SETLOCALE 1/' \
	     -e 's/#undef HAVE_STDBOOL_H$$/#define HAVE_STDBOOL_H 1/' \
	     -e 's/#undef HAVE_STDDEF_H$$/#define HAVE_STDDEF_H 1/' \
	     -e 's/#undef HAVE_STDLIB_H$$/#define HAVE_STDLIB_H 1/' \
	     -e 's/#undef HAVE_STRCASECMP$$/#define HAVE_STRCASECMP 1/' \
	     -e 's/#undef HAVE_STRDUP$$/#define HAVE_STRDUP 1/' \
	     -e 's/#undef HAVE_STRERROR$$/#define HAVE_STRERROR 1/' \
	     -e 's/#undef HAVE_STRINGS_H$$/#define HAVE_STRINGS_H 1/' \
	     -e 's/#undef HAVE_STRING_H$$/#define HAVE_STRING_H 1/' \
	     -e 's/#undef HAVE_STRTOUL$$/#define HAVE_STRTOUL 1/' \
	     -e 's/#undef HAVE_SYS_PARAM_H$$/#define HAVE_SYS_PARAM_H 1/' \
	     -e 's/#undef HAVE_SYS_STAT_H$$/#define HAVE_SYS_STAT_H 1/' \
	     -e 's/#undef HAVE_SYS_TYPES_H$$/#define HAVE_SYS_TYPES_H 1/' \
	     -e 's/#undef HAVE_UNISTD_H$$/#define HAVE_UNISTD_H 1/' \
	     -e 's/#undef HAVE_UNSETENV$$/#define HAVE_UNSETENV 1/' \
	     -e 's/#undef HAVE_UNSIGNED_LONG_LONG$$/#define HAVE_UNSIGNED_LONG_LONG 1/' \
	     -e 's/#undef HAVE_VPRINTF$$/#define HAVE_VPRINTF 1/' \
	     -e 's/#undef HAVE_WCHAR_T$$/#define HAVE_WCHAR_T 1/' \
	     -e 's/#undef HAVE_WCSLEN$$/#define HAVE_WCSLEN 1/' \
	     -e 's/#undef HAVE_WINT_T$$/#define HAVE_WINT_T 1/' \
	     -e 's/#undef HAVE_WPRINTF$$/#define HAVE_WPRINTF 1/' \
	     -e 's/#undef ICONV_CONST$$/#define ICONV_CONST/' \
	     -e 's/#undef MALLOC_0_IS_NONNULL$$/#define MALLOC_0_IS_NONNULL 0/' \
	     -e 's/#undef STDC_HEADERS$$/#define STDC_HEADERS 1/' \
	     -e 's/#undef USE_UNLOCKED_IO$$/#define USE_UNLOCKED_IO 1/' \
	     -e 's/#undef __GETOPT_PREFIX$$/#define __GETOPT_PREFIX/' \
	     -e 's/#undef realpath$$/#define realpath rpl_realpath/' \
	     -e 's/#undef uintmax_t$$/#define uintmax_t unsigned long long/' \
	     -e 's/#undef PACKAGE$$/#define PACKAGE "gettext-runtime"/' \
	     -e 's/#undef VERSION$$/#define VERSION "$(VERSION)"/' \
	   < $(srcdir)/config.h.in; echo; echo '#define alloca __ALLOCA') > $@
# vms_jackets.h is a header that comes with Compaq's "porting library".
# KEEP_CRTL_SETLOCALE tells it to not override the native locale support.

intl/libgnuintl.h_vms: intl/libgnuintl.h.in
	sed -e 's,@''HAVE_POSIX_PRINTF''@,1,g ' \
	    -e 's,@''HAVE_ASPRINTF''@,0,g' \
	    -e 's,@''HAVE_SNPRINTF''@,0,g' \
	    -e 's,@''HAVE_WPRINTF''@,1,g' \
	  < $(srcdir)/intl/libgnuintl.h.in > $@

all-local: intl/libgnuintl.h_vms

EXTRA_DIST += Makefile.vms config.h_vms


# Woe32 support.

config.h.msvc: config.h.in ../version.sh
	sed -e 's/#undef ENABLE_NLS$$/#define ENABLE_NLS 1/' \
	    -e 's/#undef ENABLE_RELOCATABLE$$/#define ENABLE_RELOCATABLE 1/' \
	    -e 's/#undef HAVE_ALLOCA$$/#define HAVE_ALLOCA 1/' \
	    -e 's/#undef HAVE_DECL_STRERROR$$/#define HAVE_DECL_STRERROR 1/' \
	    -e 's/#undef HAVE_DECL__SNPRINTF$$/#define HAVE_DECL__SNPRINTF 1/' \
	    -e 's/#undef HAVE_DECL__SNWPRINTF$$/#define HAVE_DECL__SNWPRINTF 1/' \
	    -e 's/#undef HAVE_ENVIRON_DECL$$/#define HAVE_ENVIRON_DECL 1/' \
	    -e 's/#undef HAVE_ERRNO_DECL$$/#define HAVE_ERRNO_DECL 1/' \
	    -e 's/#undef HAVE_GETCWD$$/#define HAVE_GETCWD 1/' \
	    -e 's/#undef HAVE_ICONV$$/#define HAVE_ICONV 1/' \
	    -e 's/#undef HAVE_LIMITS_H$$/#define HAVE_LIMITS_H 1/' \
	    -e 's/#undef HAVE_LOCALE_H$$/#define HAVE_LOCALE_H 1/' \
	    -e 's/#undef HAVE_LONG_DOUBLE$$/#define HAVE_LONG_DOUBLE 1/' \
	    -e 's/#undef HAVE_MEMMOVE$$/#define HAVE_MEMMOVE 1/' \
	    -e 's/#undef HAVE_PTRDIFF_T$$/#define HAVE_PTRDIFF_T 1/' \
	    -e 's/#undef HAVE_PUTENV$$/#define HAVE_PUTENV 1/' \
	    -e 's/#undef HAVE_SETLOCALE$$/#define HAVE_SETLOCALE 1/' \
	    -e 's/#undef HAVE_SNPRINTF$$/#define HAVE_SNPRINTF 1/' \
	    -e 's/#undef HAVE_STDDEF_H$$/#define HAVE_STDDEF_H 1/' \
	    -e 's/#undef HAVE_STDLIB_H$$/#define HAVE_STDLIB_H 1/' \
	    -e 's/#undef HAVE_STRDUP$$/#define HAVE_STRDUP 1/' \
	    -e 's/#undef HAVE_STRERROR$$/#define HAVE_STRERROR 1/' \
	    -e 's/#undef HAVE_STRING_H$$/#define HAVE_STRING_H 1/' \
	    -e 's/#undef HAVE_STRTOUL$$/#define HAVE_STRTOUL 1/' \
	    -e 's/#undef HAVE_SYS_STAT_H$$/#define HAVE_SYS_STAT_H 1/' \
	    -e 's/#undef HAVE_SYS_TYPES_H$$/#define HAVE_SYS_TYPES_H 1/' \
	    -e 's/#undef HAVE_VPRINTF$$/#define HAVE_VPRINTF 1/' \
	    -e 's/#undef HAVE_WCHAR_T$$/#define HAVE_WCHAR_T 1/' \
	    -e 's/#undef HAVE_WCSLEN$$/#define HAVE_WCSLEN 1/' \
	    -e 's/#undef HAVE_WINT_T$$/#define HAVE_WINT_T 1/' \
	    -e 's/#undef HAVE_WPRINTF$$/#define HAVE_WPRINTF 1/' \
	    -e 's/#undef ICONV_CONST$$/#define ICONV_CONST const/' \
	    -e 's/#undef MALLOC_0_IS_NONNULL$$/#define MALLOC_0_IS_NONNULL 1/' \
	    -e 's/#undef SIZE_MAX$$/#define SIZE_MAX 4294967295U/' \
	    -e 's/#undef STACK_DIRECTION$$/#define STACK_DIRECTION -1/' \
	    -e 's/#undef STDC_HEADERS$$/#define STDC_HEADERS 1/' \
	    -e 's/#undef USE_UNLOCKED_IO$$/#define USE_UNLOCKED_IO 1/' \
	    -e 's/#undef __GETOPT_PREFIX$$/#define __GETOPT_PREFIX/' \
	    -e 's/#undef inline$$/#define inline __inline/' \
	    -e 's/#undef ssize_t$$/#define ssize_t int/' \
	    -e 's/#undef uintmax_t$$/#define uintmax_t unsigned long/' \
	    -e 's/#undef PACKAGE$$/#define PACKAGE "gettext-runtime"/' \
	    -e 's/#undef VERSION$$/#define VERSION "$(VERSION)"/' \
	    -e '/#undef INSTALLPREFIX$$/d' \
	  < $(srcdir)/config.h.in > $@

intl/libgnuintl.h.msvc-static: intl/libgnuintl.h.in
	sed -e 's,@''HAVE_POSIX_PRINTF''@,0,g ' \
	    -e 's,@''HAVE_ASPRINTF''@,0,g' \
	    -e 's,@''HAVE_SNPRINTF''@,0,g' \
	    -e 's,@''HAVE_WPRINTF''@,1,g' \
	  < $(srcdir)/intl/libgnuintl.h.in > $@

intl/libgnuintl.h.msvc-shared: intl/libgnuintl.h.in windows/dllexport.h
	sed -e 's,@''HAVE_POSIX_PRINTF''@,0,g ' \
	    -e 's,@''HAVE_ASPRINTF''@,0,g' \
	    -e 's,@''HAVE_SNPRINTF''@,0,g' \
	    -e 's,@''HAVE_WPRINTF''@,1,g' \
	    -e 's/extern \([^"]\)/extern LIBINTL_DLL_EXPORTED \1/' \
	    -e '/#define _LIBINTL_H/r windows/dllexport.h' \
	  < $(srcdir)/intl/libgnuintl.h.in > $@

all-local: intl/libgnuintl.h.msvc-static intl/libgnuintl.h.msvc-shared

EXTRA_DIST += README.woe32 Makefile.msvc config.h.msvc windows/dllexport.h windows/intl.rc
