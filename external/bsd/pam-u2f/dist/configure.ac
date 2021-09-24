#  Copyright (C) 2014-2019 Yubico AB
AC_PREREQ([2.65])
AC_INIT([pam_u2f], [1.2.0], [https://github.com/Yubico/pam-u2f/issues],
  [pam_u2f], [https://developers.yubico.com/pam-u2f/])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([1.11 foreign subdir-objects])

AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

AC_USE_SYSTEM_EXTENSIONS
AM_PROG_CC_C_O
AM_PROG_AR
AC_LIBTOOL_WIN32_DLL
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC_STDC

AC_ARG_ENABLE([man],
  [AS_HELP_STRING([--disable-man], [Disable man page generation])],
  [:],
  [enable_man=yes]
  )
AS_IF([test "$enable_man" = "yes"],
  [AM_MISSING_PROG([A2X], a2x, $missing_dir)]
)
AM_CONDITIONAL([ENABLE_MAN], [test "$enable_man" = "yes"])

AC_ARG_ENABLE([fuzzing],
  [AS_HELP_STRING([--enable-fuzzing], [Enable fuzzing targets])]
)
AS_IF([test "$enable_fuzzing" = "yes"],[
  AC_DEFINE([WITH_FUZZING])
])
AM_CONDITIONAL([ENABLE_FUZZING], [test "$enable_fuzzing" = "yes"])

AC_CHECK_HEADERS([security/pam_appl.h], [],
  [AC_MSG_ERROR([[PAM header files not found, install libpam-dev.]])])
AC_CHECK_HEADERS([security/pam_modules.h security/_pam_macros.h security/pam_modutil.h security/openpam.h], [], [],
  [#include <sys/types.h>
   #include <security/pam_appl.h>])
AC_CHECK_LIB([pam], [pam_start])
AC_CHECK_FUNCS([pam_modutil_drop_priv openpam_borrow_cred])

case "$host" in
     *darwin*)  PAMDIR="/usr/lib/pam";;
     *linux*)   PAMDIR="/lib/x86_64-linux-gnu/security";;
     *)         PAMDIR="/usr/lib"
esac
AC_ARG_WITH(pam-dir,
  AC_HELP_STRING([--with-pam-dir=DIR], [Where to install the PAM module]), [
    case "${withval}" in
      /*) PAMDIR="${withval}";;
      ./*|../*) AC_MSG_ERROR(Bad value for --with-pam-dir);;
      *);;
  esac
])
AC_SUBST(PAMDIR, "$PAMDIR")


PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto], [], [])
PKG_CHECK_MODULES([LIBFIDO2], [libfido2 >= 1.3.0], [], [])


# Check for secure_getenv, strlcpy, readpassphrase, explicit_bzero, and memset_s
am_save_CFLAGS="$CFLAGS"
am_save_LIBS="$LIBS"
CFLAGS="$CFLAGS"
LIBS="$LIBS"
AC_CHECK_FUNCS([secure_getenv strlcpy readpassphrase explicit_bzero memset_s])
CFLAGS=$am_save_CFLAGS
LIBS=$am_save_LIBS

# Make clang emit errors for unknown warnings to make the AX_CHECK_COMPILE_FLAG
# macro behave as intended, excluding unsupported flags.
AX_CHECK_COMPILE_FLAG([-Werror=unknown-warning-option], [check_extra_flags="-Werror=unknown-warning-option"])

AC_ARG_VAR([CWFLAGS], [Warning flags])
AX_CHECK_COMPILE_FLAG([-Wall], [CWFLAGS="-Wall"], [], [$check_extra_flags])
AX_CHECK_COMPILE_FLAG([-Wextra], [CWFLAGS="$CWFLAGS -Wextra"], [], [$check_extra_flags])
AX_CHECK_COMPILE_FLAG([-Wconversion], [CWFLAGS="$CWFLAGS -Wconversion"], [], [$check_extra_flags])
# Because pam headers are doing sign-conversion, see PAM_MODUTIL_DEF_PRIVS in pam_modutil.h
AX_CHECK_COMPILE_FLAG([-Wconversion], [CWFLAGS="$CWFLAGS -Wno-sign-conversion"], [], [$check_extra_flags])
AX_CHECK_COMPILE_FLAG([-Wpedantic], [CWFLAGS="$CWFLAGS -Wpedantic"], [], [$check_extra_flags])
AX_CHECK_COMPILE_FLAG([-Wformat=2], [CWFLAGS="$CWFLAGS -Wformat=2"], [], [$check_extra_flags])
AX_CHECK_COMPILE_FLAG([-Wstrict-prototypes], [CWFLAGS="$CWFLAGS -Wstrict-prototypes"], [], [$check_extra_flags])
AX_CHECK_COMPILE_FLAG([-Wmissing-declarations], [CWFLAGS="$CWFLAGS -Wmissing-declarations"], [], [$check_extra_flags])
AX_CHECK_COMPILE_FLAG([-Wmissing-prototypes], [CWFLAGS="$CWFLAGS -Wmissing-prototypes"], [], [$check_extra_flags])
AX_CHECK_COMPILE_FLAG([-Wnull-dereference], [CWFLAGS="$CWFLAGS -Wnull-dereference"], [], [$check_extra_flags])
AX_CHECK_COMPILE_FLAG([-Wshadow], [CWFLAGS="$CWFLAGS -Wshadow"], [], [$check_extra_flags])
AX_CHECK_COMPILE_FLAG([-Wpointer-arith], [CWFLAGS="$CWFLAGS -Wpointer-arith"], [], [$check_extra_flags])

AC_ARG_VAR([CSFLAGS], [Warning suppression flags])
AX_CHECK_COMPILE_FLAG([-Wno-unused-but-set-variable], [CSFLAGS="-Wno-unused-but-set-variable"], [], [$check_extra_flags])

AC_CONFIG_FILES([
  Makefile
  pamu2fcfg/Makefile
  tests/Makefile
  fuzz/Makefile
])

creduser=$(whoami)
AC_SUBST(USERNAME, $creduser)
AC_CONFIG_FILES([tests/credentials/new_double_-N.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-P-N.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-P.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-P-V-N.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-P-V.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-r-N.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-r-P-N.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-r-P.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-r-P-V-N.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-r-P-V.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-r.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-r-V-N.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-r-V.cred])
AC_CONFIG_FILES([tests/credentials/new_double_.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-V-N.cred])
AC_CONFIG_FILES([tests/credentials/new_double_-V.cred])
AC_CONFIG_FILES([tests/credentials/new_mixed_12.cred])
AC_CONFIG_FILES([tests/credentials/new_mixed_1-P2.cred])
AC_CONFIG_FILES([tests/credentials/new_mixed_-P12.cred])
AC_CONFIG_FILES([tests/credentials/new_mixed_-P1-P2.cred])
AC_CONFIG_FILES([tests/credentials/new_-N.cred])
AC_CONFIG_FILES([tests/credentials/new_-P-N.cred])
AC_CONFIG_FILES([tests/credentials/new_-P.cred])
AC_CONFIG_FILES([tests/credentials/new_-P-V-N.cred])
AC_CONFIG_FILES([tests/credentials/new_-P-V.cred])
AC_CONFIG_FILES([tests/credentials/new_-r-N.cred])
AC_CONFIG_FILES([tests/credentials/new_-r-P-N.cred])
AC_CONFIG_FILES([tests/credentials/new_-r-P.cred])
AC_CONFIG_FILES([tests/credentials/new_-r-P-V-N.cred])
AC_CONFIG_FILES([tests/credentials/new_-r-P-V.cred])
AC_CONFIG_FILES([tests/credentials/new_-r.cred])
AC_CONFIG_FILES([tests/credentials/new_-r-V-N.cred])
AC_CONFIG_FILES([tests/credentials/new_-r-V.cred])
AC_CONFIG_FILES([tests/credentials/new_.cred])
AC_CONFIG_FILES([tests/credentials/new_-V-N.cred])
AC_CONFIG_FILES([tests/credentials/new_-V.cred])
AC_CONFIG_FILES([tests/credentials/old_credential.cred])
AC_CONFIG_FILES([tests/credentials/ssh_credential.cred])
AC_OUTPUT


AC_MSG_NOTICE([Summary of build options:

  Version:             ${VERSION}
  Host type:           ${host}
  Install prefix:      ${prefix}
  Compiler:            ${CC}
  CFLAGS:              ${CFLAGS}
  CWFLAGS:             ${CWFLAGS}
  CSFLAGS:             ${CSFLAGS}
  Library types:       Shared=${enable_shared}, Static=${enable_static}
  LIBFIDO2 CFLAGS:     $LIBFIDO2_CFLAGS
  LIBFIDO2 LIBS:       $LIBFIDO2_LIBS
  LIBCRYPTO CFLAGS:    $LIBCRYPTO_CFLAGS
  LIBCRYPTO LIBS:      $LIBCRYPTO_LIBS
  PAMDIR:              $PAMDIR
])
