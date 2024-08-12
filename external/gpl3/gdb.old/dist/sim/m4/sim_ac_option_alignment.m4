dnl Copyright (C) 1997-2023 Free Software Foundation, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl
dnl Specify the alignment restrictions of the target architecture.
dnl Without this option all possible alignment restrictions are accommodated.
AC_DEFUN([SIM_AC_OPTION_ALIGNMENT],
[dnl
AC_MSG_CHECKING([whether to force sim alignment])
sim_alignment=
AC_ARG_ENABLE(sim-alignment,
[AS_HELP_STRING([--enable-sim-alignment=align],
		[Specify strict, nonstrict or forced alignment of memory accesses])],
[case "${enableval}" in
  yes | strict | STRICT)      sim_alignment="STRICT_ALIGNMENT";;
  no | nonstrict | NONSTRICT) sim_alignment="NONSTRICT_ALIGNMENT";;
  forced | FORCED)            sim_alignment="FORCED_ALIGNMENT";;
  *) AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-alignment");;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_ALIGNMENT], [${sim_alignment:-0}], [Sim alignment settings])
AC_MSG_RESULT([${sim_alignment:-no}])
])dnl
