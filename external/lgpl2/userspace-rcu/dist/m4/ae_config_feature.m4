# SPDX-FileCopyrightText: 2020 Michael Jeanson <mjeanson@efficios.com>
# SPDX-FileCopyrightText: 2008 Francesco Salvestrini <salvestrini@users.sourceforge.net>
#
# SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Autoconf-exception-macro
#
# SYNOPSIS
#
#   AE_FEATURE(FEATURE-NAME, FEATURE-DESCRIPTION,
#              ACTION-IF-GIVEN?, ACTION-IF-NOT-GIVEN?,
#              ACTION-IF-ENABLED?, ACTION-IF-NOT-ENABLED?)
#
# DESCRIPTION
#
#   AE_FEATURE is a simple wrapper for AC_ARG_ENABLE.
#
#   FEATURE-NAME should consist only of alphanumeric characters, dashes,
#   plus signs, and dots.
#
#   FEATURE-DESCRIPTION will be formatted with AS_HELP_STRING.
#
#   If the user gave configure the option --enable-FEATURE or --disable-FEATURE,
#   run shell commands ACTION-IF-GIVEN. If neither option was given, run shell
#   commands ACTION-IF-NOT-GIVEN. The name feature indicates an optional
#
#   If the feature is enabled, run shell commands ACTION-IF-ENABLED, if it is
#   disabled, run shell commands ACTION-IF-NOT-ENABLED.
#
#   A FEATURE has 3 different states, enabled, disabled and undefined. The first
#   two are self explanatory, the third state means it's disabled by default
#   and it was not explicitly enabled or disabled by the user or by the
#   AE_FEATURE_ENABLE and AE_FEATURE_DISABLE macros.
#
#   A feature is disabled by default, in order to change this behaviour use the
#   AE_FEATURE_DEFAULT_ENABLE and AE_FEATURE_DEFAULT_DISABLE
#   macros.
#
#   A simple example:
#
#     AE_FEATURE_DEFAULT_ENABLE
#     AE_FEATURE(feature_xxxxx, [turns on/off XXXXX support])
#
#     ...
#
#     AE_FEATURE_DEFAULT_DISABLE
#     AE_FEATURE(feature_yyyyy, [turns on/off YYYYY support])
#     AM_CONDITIONAL(YYYYY, AE_IS_FEATURE_ENABLED([feature_yyyyy]))
#
#     AE_FEATURE_DEFAULT_ENABLE
#     AE_FEATURE(...)
#
#     ...
#
#   Use AE_FEATURE_ENABLE or AE_FEATURE_DISABLE in order to
#   enable or disable a specific feature.
#
#   Another simple example:
#
#     AS_IF([some_test_here],[AE_FEATURE_ENABLE(feature_xxxxx)],[])
#
#     AE_FEATURE(feature_xxxxx, [turns on/off XXXXX support],
#                       HAVE_XXXXX, [Define if you want XXXXX support])
#     AE_FEATURE(feature_yyyyy, [turns on/off YYYYY support],
#                       HAVE_YYYYY, [Define if you want YYYYY support])
#
#     ...
#
#   NOTE: AE_FEATURE_ENABLE/DISABLE() must be placed first of the relative
#   AE_FEATURE() macro if you want the the proper ACTION-IF-ENABLED and
#   ACTION-IF-NOT-ENABLED to run.

#serial 3


# AE_FEATURE_DEFAULT_ENABLE: The next feature defined with AE_FEATURE will
# default to enable.
AC_DEFUN([AE_FEATURE_DEFAULT_ENABLE], [
  m4_define([ae_feature_default_arg], [yes])
  m4_define([ae_feature_default_switch], [disable])
])


# AE_FEATURE_DEFAULT_DISABLE: The next feature defined with AE_FEATURE will
# default to disable.
#
AC_DEFUN([AE_FEATURE_DEFAULT_DISABLE], [
  m4_define([ae_feature_default_arg], [no])
  m4_define([ae_feature_default_switch], [enable])
])


# AE_FEATURE_ENABLE(FEATURE-NAME): Enable the FEATURE, this will override the
# user's choice if it was made.
#
AC_DEFUN([AE_FEATURE_ENABLE],[ dnl
  enable_[]patsubst([$1], -, _)[]=yes
])


# AE_FEATURE_DISABLE(FEATURE-NAME): Disable the FEATURE, this will override the
# user's choice if it was made.
#
AC_DEFUN([AE_FEATURE_DISABLE],[ dnl
  enable_[]patsubst([$1], -, _)[]=no
])


# AE_IF_FEATURE_ENABLED(FEATURE-NAME, ACTION-IF-ENABLED, ACTION-IF-NOT-ENABLED?):
# Run shell code ACTION-IF-ENABLED if the FEATURE is enabled, otherwise run
# shell code ACTION-IF-NOT-ENABLED.
#
AC_DEFUN([AE_IF_FEATURE_ENABLED],[ dnl
m4_pushdef([FEATURE], patsubst([$1], -, _))dnl

  AS_IF([test "$enable_[]FEATURE[]" = yes],[ dnl
    $2
  ],[: dnl
    $3
  ])
])


# AE_IF_FEATURE_NOT_ENABLED(FEATURE-NAME, ACTION-IF-NOT-ENABLED,
#                           ACTION-IF-NOT-NOT-ENABLED?):
# Run shell code ACTION-IF-NOT-ENABLED if the FEATURE is not explicitly
# enabled, otherwise run shell code ACTION-IF-NOT-NOT-DISABLED.
#
# The distinction with AE_IF_FEATURE_DISABLED is that this will also
# match a feature that is undefined.
#
# A feature is undefined when it's disabled by default and was not explicitly
# enabled or disabled by the user or by AE_FEATURE_ENABLE/DISABLE.
#
AC_DEFUN([AE_IF_FEATURE_NOT_ENABLED],[ dnl
m4_pushdef([FEATURE], patsubst([$1], -, _))dnl

  AS_IF([test "$enable_[]FEATURE[]" != yes],[ dnl
    $2
  ],[: dnl
    $3
  ])
])


# AE_IF_FEATURE_DISABLED(FEATURE-NAME, ACTION-IF-DISABLED, ACTION-IF-NOT-DISABLED?):
# Run shell code ACTION-IF-DISABLED if the FEATURE is disabled, otherwise run
# shell code ACTION-IF-NOT-DISABLED.
#
AC_DEFUN([AE_IF_FEATURE_DISABLED],[ dnl
m4_pushdef([FEATURE], patsubst([$1], -, _))dnl

  AS_IF([test "$enable_[]FEATURE[]" = no],[ dnl
    $2
  ],[: dnl
    $3
  ])
])


# AE_IF_FEATURE_UNDEF(FEATURE-NAME, ACTION-IF-UNDEF, ACTION-IF-NOT-UNDEF?):
# Run shell code ACTION-IF-UNDEF if the FEATURE is undefined, otherwise run
# shell code ACTION-IF-NOT-UNDEF.
#
# A feature is undefined when it's disabled by default and was not explicitly
# enabled or disabled by the user or by AE_FEATURE_ENABLE/DISABLE.
#
AC_DEFUN([AE_IF_FEATURE_UNDEF],[ dnl
m4_pushdef([FEATURE], patsubst([$1], -, _))dnl

  AS_IF([test "x$enable_[]FEATURE[]" = x],[ dnl
    $2
  ],[: dnl
    $3
  ])
])


# AE_IS_FEATURE_ENABLED(FEATURE-NAME): outputs a shell condition (suitable
# for use in a shell if statement) that will return true if the FEATURE is
# enabled.
#
AC_DEFUN([AE_IS_FEATURE_ENABLED],[dnl
m4_pushdef([FEATURE], patsubst([$1], -, _))dnl
 test "x$enable_[]FEATURE[]" = xyes dnl
])


dnl Disabled by default, unless already overridden
m4_ifndef([ae_feature_default_arg],[AE_FEATURE_DEFAULT_DISABLE])


# AE_FEATURE(FEATURE-NAME, FEATURE-DESCRIPTION,
#            ACTION-IF-GIVEN?, ACTION-IF-NOT-GIVEN?,
#            ACTION-IF-ENABLED?, ACTION-IF-NOT-ENABLED?):
#
#
AC_DEFUN([AE_FEATURE],[ dnl
m4_pushdef([FEATURE], patsubst([$1], -, _))dnl

dnl If the option wasn't specified and the default is enabled, set enable_FEATURE to yes
AS_IF([test "x$enable_[]FEATURE[]" = x && test "ae_feature_default_arg" = yes],[ dnl
  enable_[]FEATURE[]="ae_feature_default_arg"
])

AC_ARG_ENABLE([$1],
  AS_HELP_STRING([--ae_feature_default_switch-$1],dnl
                 [$2]),[
case "${enableval}" in
   yes)
     enable_[]FEATURE[]=yes
     ;;
   no)
     enable_[]FEATURE[]=no
     ;;
   *)
     AC_MSG_ERROR([bad value ${enableval} for feature --$1])
     ;;
esac

$3
],[: dnl
$4
])

AS_IF([test "$enable_[]FEATURE[]" = yes],[: dnl
  $5
],[: dnl
  $6
])

m4_popdef([FEATURE])dnl
])
