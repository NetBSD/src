dnl ######################################################################
dnl What directory path separator do we use?
AC_DEFUN([NTP_DIR_SEP], [
AC_CACHE_CHECK([for directory path separator], ac_cv_dir_sep,
[
  case "$ac_cv_dir_sep" in
   '')
    case "$target_os" in
      *djgpp | *mingw32* | *emx*) ac_cv_dir_sep="'\\'" ;;
      *) ac_cv_dir_sep="'/'" ;;
    esac
    ;;
  esac
])
AC_DEFINE_UNQUOTED(DIR_SEP,$ac_cv_dir_sep,dnl
   [Directory separator character, usually / or \\])dnl
])
dnl ======================================================================
