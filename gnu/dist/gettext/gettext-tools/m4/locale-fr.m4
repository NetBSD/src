# locale-fr.m4 serial 2 (gettext-0.14.2)
dnl Copyright (C) 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

dnl Determine the name of a french locale with traditional encoding.
AC_DEFUN([gt_LOCALE_FR],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_CACHE_CHECK([for a traditional french locale], gt_cv_locale_fr, [
    macosx=
    case "$host_os" in
      darwin[56]*) ;;
      darwin*) macosx=yes;;
    esac
    if test -n "$macosx"; then
      # On Darwin 7 (MacOS X), the libc supports some locales in non-UTF-8
      # encodings, but the kernel does not support them. The documentation
      # says:
      #   "... all code that calls BSD system routines should ensure
      #    that the const *char parameters of these routines are in UTF-8
      #    encoding. All BSD system functions expect their string
      #    parameters to be in UTF-8 encoding and nothing else."
      # See the comments in config.charset. Therefore we bypass the test.
      gt_cv_locale_fr=none
    else
changequote(,)dnl
      cat <<EOF > conftest.$ac_ext
#include <locale.h>
#include <time.h>
struct tm t;
char buf[16];
int main () {
  /* Check whether the given locale name is recognized by the system.  */
  if (setlocale (LC_ALL, "") == NULL) return 1;
  /* Check whether in the abbreviation of the second month, the second
     character (should be U+00E9: LATIN SMALL LETTER E WITH ACUTE) is only
     one byte long. This excludes the UTF-8 encoding.  */
  t.tm_year = 1975 - 1900; t.tm_mon = 2 - 1; t.tm_mday = 4;
  if (strftime (buf, sizeof (buf), "%b", &t) < 3 || buf[2] != 'v') return 1;
  return 0;
}
EOF
changequote([,])dnl
      if AC_TRY_EVAL([ac_link]) && test -s conftest$ac_exeext; then
        # Setting LC_ALL is not enough. Need to set LC_TIME to empty, because
        # otherwise on MacOS X 10.3.5 the LC_TIME=C from the beginning of the
        # configure script would override the LC_ALL setting.
        # Test for the usual locale name.
        if (LC_ALL=fr_FR LC_TIME= ./conftest; exit) 2>/dev/null; then
          gt_cv_locale_fr=fr_FR
        else
          # Test for the locale name with explicit encoding suffix.
          if (LC_ALL=fr_FR.ISO-8859-1 LC_TIME= ./conftest; exit) 2>/dev/null; then
            gt_cv_locale_fr=fr_FR.ISO-8859-1
          else
            # Test for the AIX, OSF/1, FreeBSD, NetBSD locale name.
            if (LC_ALL=fr_FR.ISO8859-1 LC_TIME= ./conftest; exit) 2>/dev/null; then
              gt_cv_locale_fr=fr_FR.ISO8859-1
            else
              # Test for the HP-UX locale name.
              if (LC_ALL=fr_FR.iso88591 LC_TIME= ./conftest; exit) 2>/dev/null; then
                gt_cv_locale_fr=fr_FR.iso88591
              else
                # Test for the Solaris 7 locale name.
                if (LC_ALL=fr LC_TIME= ./conftest; exit) 2>/dev/null; then
                  gt_cv_locale_fr=fr
                else
                  # Special test for NetBSD 1.6.
                  if test -f /usr/share/locale/fr_FR.ISO8859-1/LC_CTYPE; then
                    gt_cv_locale_fr=fr_FR.ISO8859-1
                  else
                    # None found.
                    gt_cv_locale_fr=none
                  fi
                fi
              fi
            fi
          fi
        fi
      fi
      rm -fr conftest*
    fi
  ])
  LOCALE_FR=$gt_cv_locale_fr
  AC_SUBST([LOCALE_FR])
])

dnl Determine the name of a french locale with UTF-8 encoding.
AC_DEFUN([gt_LOCALE_FR_UTF8],
[
  AC_CACHE_CHECK([for a french Unicode locale], gt_cv_locale_fr_utf8, [
changequote(,)dnl
    cat <<EOF > conftest.$ac_ext
#include <locale.h>
#include <time.h>
struct tm t;
char buf[16];
int main () {
  /* On BeOS, locales are not implemented in libc.  Rather, libintl
     imitates locale dependent behaviour by looking at the environment
     variables, and all locales use the UTF-8 encoding.  */
#if !defined(__BEOS__)
  /* Check whether the given locale name is recognized by the system.  */
  if (setlocale (LC_ALL, "") == NULL) return 1;
  /* Check whether in the abbreviation of the second month, the second
     character (should be U+00E9: LATIN SMALL LETTER E WITH ACUTE) is
     two bytes long, with UTF-8 encoding.  */
  t.tm_year = 1975 - 1900; t.tm_mon = 2 - 1; t.tm_mday = 4;
  if (strftime (buf, sizeof (buf), "%b", &t) < 4
      || buf[1] != (char) 0xc3 || buf[2] != (char) 0xa9 || buf[3] != 'v')
    return 1;
#endif
  return 0;
}
EOF
changequote([,])dnl
    if AC_TRY_EVAL([ac_link]) && test -s conftest$ac_exeext; then
      # Setting LC_ALL is not enough. Need to set LC_TIME to empty, because
      # otherwise on MacOS X 10.3.5 the LC_TIME=C from the beginning of the
      # configure script would override the LC_ALL setting.
      # Test for the usual locale name.
      if (LC_ALL=fr_FR LC_TIME= ./conftest; exit) 2>/dev/null; then
        gt_cv_locale_fr_utf8=fr_FR
      else
        # Test for the locale name with explicit encoding suffix.
        if (LC_ALL=fr_FR.UTF-8 LC_TIME= ./conftest; exit) 2>/dev/null; then
          gt_cv_locale_fr_utf8=fr_FR.UTF-8
        else
          # Test for the Solaris 7 locale name.
          if (LC_ALL=fr.UTF-8 LC_TIME= ./conftest; exit) 2>/dev/null; then
            gt_cv_locale_fr_utf8=fr.UTF-8
          else
            # None found.
            gt_cv_locale_fr_utf8=none
          fi
        fi
      fi
    fi
    rm -fr conftest*
  ])
  LOCALE_FR_UTF8=$gt_cv_locale_fr_utf8
  AC_SUBST([LOCALE_FR_UTF8])
])
