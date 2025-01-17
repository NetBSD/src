# SPDX-FileCopyrightText: 2016 Philippe Proulx <pproulx@efficios.com>
#
# SPDX-License-Identifier: GPL-2.0-or-later WITH Autoconf-exception-2.0

#serial 2

# AE_PPRINT_INIT(): initializes the pretty printing system.
#
# Use this macro before using any other AE_PPRINT_* macro.
AC_DEFUN([AE_PPRINT_INIT], [
  m4_define([AE_PPRINT_CONFIG_TS], [50])
  m4_define([AE_PPRINT_CONFIG_INDENT], [2])
  AE_PPRINT_YES_MSG=yes
  AE_PPRINT_NO_MSG=no

  # find tput, which tells us if colors are supported and gives us color codes
  AC_PATH_PROG([ae_pprint_tput], [tput])

  AS_IF([test -n "$ae_pprint_tput"], [
    AS_IF([test -n "$PS1" && test `"$ae_pprint_tput" colors` -eq 256 && test -t 1], [
      # interactive shell and colors supported and standard output
      # file descriptor is opened on a terminal
      AE_PPRINT_COLOR_TXTBLK="`"$ae_pprint_tput" setaf 0`"
      AE_PPRINT_COLOR_TXTBLU="`"$ae_pprint_tput" setaf 4`"
      AE_PPRINT_COLOR_TXTGRN="`"$ae_pprint_tput" setaf 2`"
      AE_PPRINT_COLOR_TXTCYN="`"$ae_pprint_tput" setaf 6`"
      AE_PPRINT_COLOR_TXTRED="`"$ae_pprint_tput" setaf 1`"
      AE_PPRINT_COLOR_TXTPUR="`"$ae_pprint_tput" setaf 5`"
      AE_PPRINT_COLOR_TXTYLW="`"$ae_pprint_tput" setaf 3`"
      AE_PPRINT_COLOR_TXTWHT="`"$ae_pprint_tput" setaf 7`"
      AE_PPRINT_COLOR_BLD=`"$ae_pprint_tput" bold`
      AE_PPRINT_COLOR_BLDBLK="$AE_PPRINT_COLOR_BLD$AE_PPRINT_COLOR_TXTBLK"
      AE_PPRINT_COLOR_BLDBLU="$AE_PPRINT_COLOR_BLD$AE_PPRINT_COLOR_TXTBLU"
      AE_PPRINT_COLOR_BLDGRN="$AE_PPRINT_COLOR_BLD$AE_PPRINT_COLOR_TXTGRN"
      AE_PPRINT_COLOR_BLDCYN="$AE_PPRINT_COLOR_BLD$AE_PPRINT_COLOR_TXTCYN"
      AE_PPRINT_COLOR_BLDRED="$AE_PPRINT_COLOR_BLD$AE_PPRINT_COLOR_TXTRED"
      AE_PPRINT_COLOR_BLDPUR="$AE_PPRINT_COLOR_BLD$AE_PPRINT_COLOR_TXTPUR"
      AE_PPRINT_COLOR_BLDYLW="$AE_PPRINT_COLOR_BLD$AE_PPRINT_COLOR_TXTYLW"
      AE_PPRINT_COLOR_BLDWHT="$AE_PPRINT_COLOR_BLD$AE_PPRINT_COLOR_TXTWHT"
      AE_PPRINT_COLOR_RST="`"$ae_pprint_tput" sgr0`"

      # colored yes and no
      AE_PPRINT_YES_MSG="$AE_PPRINT_COLOR_BLDGRN$AE_PPRINT_YES_MSG$AE_PPRINT_COLOR_RST"
      AE_PPRINT_NO_MSG="$AE_PPRINT_COLOR_BLDRED$AE_PPRINT_NO_MSG$AE_PPRINT_COLOR_RST"

      # subtitle color
      AE_PPRINT_COLOR_SUBTITLE="$AE_PPRINT_COLOR_BLDCYN"
    ])
  ])
])

# AE_PPRINT_SET_INDENT(indent): sets the current indentation.
#
# Use AE_PPRINT_INIT() before using this macro.
AC_DEFUN([AE_PPRINT_SET_INDENT], [
  m4_define([AE_PPRINT_CONFIG_INDENT], [$1])
])

# AE_PPRINT_SET_TS(ts): sets the current tab stop.
#
# Use AE_PPRINT_INIT() before using this macro.
AC_DEFUN([AE_PPRINT_SET_TS], [
  m4_define([AE_PPRINT_CONFIG_TS], [$1])
])

# AE_PPRINT_SUBTITLE(subtitle): pretty prints a subtitle.
#
# The subtitle is put as is in a double-quoted shell string so the user
# needs to escape ".
#
# Use AE_PPRINT_INIT() before using this macro.
AC_DEFUN([AE_PPRINT_SUBTITLE], [
  AS_ECHO(["${AE_PPRINT_COLOR_SUBTITLE}$1$AE_PPRINT_COLOR_RST"])
])

AC_DEFUN([_AE_PPRINT_INDENT], [
  m4_if(AE_PPRINT_CONFIG_INDENT, 0, [
  ], [
    m4_for([ae_pprint_i], [0], m4_eval(AE_PPRINT_CONFIG_INDENT * 2 - 1), [1], [
      AS_ECHO_N([" "])
    ])
  ])
])

# AE_PPRINT_PROP_STRING(title, value, title_color?): pretty prints a
# string property.
#
# The title is put as is in a double-quoted shell string so the user
# needs to escape ".
#
# The $AE_PPRINT_CONFIG_INDENT variable must be set to the desired indentation
# level.
#
# Use AE_PPRINT_INIT() before using this macro.
AC_DEFUN([AE_PPRINT_PROP_STRING], [
  m4_pushdef([ae_pprint_title], [$1])
  m4_pushdef([ae_pprint_value], [$2])
  m4_pushdef([ae_pprint_title_color], m4_default([$3], []))
  m4_pushdef([ae_pprint_title_len], m4_len(ae_pprint_title))
  m4_pushdef([ae_pprint_spaces_cnt], m4_eval(AE_PPRINT_CONFIG_TS - ae_pprint_title_len - (AE_PPRINT_CONFIG_INDENT * 2) - 1))

  m4_if(m4_eval(ae_pprint_spaces_cnt <= 0), [1], [
    m4_define([ae_pprint_spaces_cnt], [1])
  ])

  m4_pushdef([ae_pprint_spaces], [])

  m4_for([ae_pprint_i], 0, m4_eval(ae_pprint_spaces_cnt - 1), [1], [
    m4_append([ae_pprint_spaces], [ ])
  ])

  _AE_PPRINT_INDENT

  AS_ECHO_N(["ae_pprint_title_color""ae_pprint_title$AE_PPRINT_COLOR_RST:ae_pprint_spaces"])
  AS_ECHO(["${AE_PPRINT_COLOR_BLD}ae_pprint_value$AE_PPRINT_COLOR_RST"])

  m4_popdef([ae_pprint_spaces])
  m4_popdef([ae_pprint_spaces_cnt])
  m4_popdef([ae_pprint_title_len])
  m4_popdef([ae_pprint_title_color])
  m4_popdef([ae_pprint_value])
  m4_popdef([ae_pprint_title])
])

# AE_PPRINT_PROP_BOOL(title, value, title_color?): pretty prints a boolean
# property.
#
# The title is put as is in a double-quoted shell string so the user
# needs to escape ".
#
# The value is evaluated at shell runtime. Its evaluation must be
# 0 (false) or 1 (true).
#
# Uses the AE_PPRINT_PROP_STRING() with the "yes" or "no" string.
#
# Use AE_PPRINT_INIT() before using this macro.
AC_DEFUN([AE_PPRINT_PROP_BOOL], [
  m4_pushdef([ae_pprint_title], [$1])
  m4_pushdef([ae_pprint_value], [$2])

  test ae_pprint_value -eq 0 && ae_pprint_msg="$AE_PPRINT_NO_MSG" || ae_pprint_msg="$AE_PPRINT_YES_MSG"

  m4_if([$#], [3], [
    AE_PPRINT_PROP_STRING(ae_pprint_title, [$ae_pprint_msg], $3)
  ], [
    AE_PPRINT_PROP_STRING(ae_pprint_title, [$ae_pprint_msg])
  ])

  m4_popdef([ae_pprint_value])
  m4_popdef([ae_pprint_title])
])

# AE_PPRINT_PROP_BOOL_CUSTOM(title, value, no_msg, title_color?): pretty prints a boolean
# property.
#
# The title is put as is in a double-quoted shell string so the user
# needs to escape ".
#
# The value is evaluated at shell runtime. Its evaluation must be
# 0 (false) or 1 (true).
#
# Uses the AE_PPRINT_PROP_STRING() with the "yes" or "no" string.
#
# Use AE_PPRINT_INIT() before using this macro.
AC_DEFUN([AE_PPRINT_PROP_BOOL_CUSTOM], [
  m4_pushdef([ae_pprint_title], [$1])
  m4_pushdef([ae_pprint_value], [$2])
  m4_pushdef([ae_pprint_value_no_msg], [$3])

  test ae_pprint_value -eq 0 && ae_pprint_msg="$AE_PPRINT_NO_MSG (ae_pprint_value_no_msg)" || ae_pprint_msg="$AE_PPRINT_YES_MSG"

  m4_if([$#], [4], [
    AE_PPRINT_PROP_STRING(ae_pprint_title, [$ae_pprint_msg], $4)
  ], [
    AE_PPRINT_PROP_STRING(ae_pprint_title, [$ae_pprint_msg])
  ])

  m4_popdef([ae_pprint_value_no_msg])
  m4_popdef([ae_pprint_value])
  m4_popdef([ae_pprint_title])
])

# AE_PPRINT_WARN(msg): pretty prints a warning message.
#
# The message is put as is in a double-quoted shell string so the user
# needs to escape ".
#
# Use AE_PPRINT_INIT() before using this macro.
AC_DEFUN([AE_PPRINT_WARN], [
  m4_pushdef([ae_pprint_msg], [$1])

  _AE_PPRINT_INDENT
  AS_ECHO(["${AE_PPRINT_COLOR_TXTYLW}WARNING:$AE_PPRINT_COLOR_RST ${AE_PPRINT_COLOR_BLDYLW}ae_pprint_msg$AE_PPRINT_COLOR_RST"])

  m4_popdef([ae_pprint_msg])
])

# AE_PPRINT_ERROR(msg): pretty prints an error message and exits.
#
# The message is put as is in a double-quoted shell string so the user
# needs to escape ".
#
# Use AE_PPRINT_INIT() before using this macro.
AC_DEFUN([AE_PPRINT_ERROR], [
  m4_pushdef([ae_pprint_msg], [$1])

  AC_MSG_ERROR([${AE_PPRINT_COLOR_BLDRED}ae_pprint_msg$AE_PPRINT_COLOR_RST])

  m4_popdef([ae_pprint_msg])
])
