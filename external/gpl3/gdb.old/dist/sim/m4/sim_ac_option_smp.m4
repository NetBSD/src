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
AC_DEFUN([SIM_AC_OPTION_SMP],
[
AC_MSG_CHECKING([number of sim cpus to support])
default_sim_smp="ifelse([$1],,5,[$1])"
sim_smp="$default_sim_smp""
AC_ARG_ENABLE(sim-smp,
[AS_HELP_STRING([--enable-sim-smp=n],
		[Specify number of processors to configure for (default ${default_sim_smp})])],
[case "${enableval}" in
  yes)	sim_smp="5";;
  no)	sim_smp="0";;
  *)	sim_smp="$enableval";;
esac])dnl
sim_igen_smp="-N ${sim_smp}"
AC_DEFINE_UNQUOTED([WITH_SMP], [$sim_smp], [Sim SMP settings])
AC_MSG_RESULT($sim_smp)
])
