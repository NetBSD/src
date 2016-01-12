# openmp.m4 serial 3 (gettext-0.16)
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Determine the compiler flags needed to support OpenMP.
dnl Define OPENMP_CFLAGS.

dnl From Bruno Haible.

AC_DEFUN([gt_OPENMP],
[
  AC_MSG_CHECKING([whether to use OpenMP])
  AC_ARG_ENABLE(openmp,
    [  --disable-openmp        do not use OpenMP],
    [OPENMP_CHOICE="$enableval"],
    [OPENMP_CHOICE=yes])
  AC_MSG_RESULT([$OPENMP_CHOICE])
  OPENMP_CFLAGS=
  if test "$OPENMP_CHOICE" = yes; then
    AC_MSG_CHECKING([for $CC option to support OpenMP])
    AC_CACHE_VAL([gt_cv_prog_cc_openmp], [
      gt_cv_prog_cc_openmp=no
      AC_COMPILE_IFELSE([
#ifndef _OPENMP
 Unlucky
#endif
        ], [gt_cv_prog_cc_openmp=none])
      if test "$gt_cv_prog_cc_openmp" = no; then
        dnl Try these flags:
        dnl   GCC >= 4.2           -fopenmp
        dnl   SunPRO C             -xopenmp
        dnl   Intel C              -openmp
        dnl   SGI C, PGI C         -mp
        dnl   Tru64 Compaq C       -omp
        dnl   AIX IBM C            -qsmp=omp
        if test "$GCC" = yes; then
          dnl --- Test for GCC.
          gt_save_CFLAGS="$CFLAGS"
          CFLAGS="$CFLAGS -fopenmp"
          AC_COMPILE_IFELSE([
#ifndef _OPENMP
 Unlucky
#endif
            ], [gt_cv_prog_cc_openmp="-fopenmp"])
          CFLAGS="$gt_save_CFLAGS"
        else
          dnl --- Test for SunPRO C.
          AC_EGREP_CPP([Brand], [
#if defined __SUNPRO_C || defined __SUNPRO_CC
 Brand
#endif
            ], result=yes, result=no)
          if test $result = yes; then
            gt_save_CFLAGS="$CFLAGS"
            CFLAGS="$CFLAGS -xopenmp"
            AC_COMPILE_IFELSE([
#ifndef _OPENMP
 Unlucky
#endif
              ], [gt_cv_prog_cc_openmp="-xopenmp"])
            CFLAGS="$gt_save_CFLAGS"
          else
            dnl --- Test for Intel C.
            AC_EGREP_CPP([Brand], [
#if defined __INTEL_COMPILER
 Brand
#endif
              ], result=yes, result=no)
            if test $result = yes; then
              gt_save_CFLAGS="$CFLAGS"
              CFLAGS="$CFLAGS -openmp"
              AC_COMPILE_IFELSE([
#ifndef _OPENMP
 Unlucky
#endif
                ], [gt_cv_prog_cc_openmp="-openmp"])
              CFLAGS="$gt_save_CFLAGS"
            else
              dnl --- Test for SGI C, PGI C.
              AC_EGREP_CPP([Brand], [
#if defined __sgi || defined __PGI || defined __PGIC__
 Brand
#endif
                ], result=yes, result=no)
              if test $result = yes; then
                gt_save_CFLAGS="$CFLAGS"
                CFLAGS="$CFLAGS -mp"
                AC_COMPILE_IFELSE([
#ifndef _OPENMP
 Unlucky
#endif
                  ], [gt_cv_prog_cc_openmp="-mp"])
                CFLAGS="$gt_save_CFLAGS"
              else
                dnl --- Test for Compaq C.
                AC_EGREP_CPP([Brand], [
#if defined __DECC || defined __DECCXX
 Brand
#endif
                  ], result=yes, result=no)
                if test $result = yes; then
                  gt_save_CFLAGS="$CFLAGS"
                  CFLAGS="$CFLAGS -omp"
                  AC_COMPILE_IFELSE([
#ifndef _OPENMP
 Unlucky
#endif
                    ], [gt_cv_prog_cc_openmp="-omp"])
                  CFLAGS="$gt_save_CFLAGS"
                else
                  dnl --- Test for AIX IBM C.
                  AC_EGREP_CPP([Brand], [
#if defined _AIX
 Brand
#endif
                    ], result=yes, result=no)
                  if test $result = yes; then
                    gt_save_CFLAGS="$CFLAGS"
                    CFLAGS="$CFLAGS -qsmp=omp"
                    AC_COMPILE_IFELSE([
#ifndef _OPENMP
 Unlucky
#endif
                      ], [gt_cv_prog_cc_openmp="-qsmp=omp"])
                    CFLAGS="$gt_save_CFLAGS"
                  else
                    :
                  fi
                fi
              fi
            fi
          fi
        fi
      fi
      ])
    case $gt_cv_prog_cc_openmp in
      none)
        AC_MSG_RESULT([none needed]) ;;
      no)
        AC_MSG_RESULT([unsupported]) ;;
      *)
        AC_MSG_RESULT([$gt_cv_prog_cc_openmp]) ;;
    esac
    case $gt_cv_prog_cc_openmp in
      none | no)
        OPENMP_CFLAGS= ;;
      *)
        OPENMP_CFLAGS=$gt_cv_prog_cc_openmp ;;
    esac
  fi
  AC_SUBST([OPENMP_CFLAGS])
])
