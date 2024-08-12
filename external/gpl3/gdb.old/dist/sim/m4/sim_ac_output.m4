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
dnl Generate the Makefile in a target specific directory.
dnl Substitutions aren't performed on the file in AC_SUBST_FILE,
dnl so this is a cover macro to tuck the details away of how we cope.
dnl We cope by having autoconf generate two files and then merge them into
dnl one afterwards.  The two pieces of the common fragment are inserted into
dnl the target's fragment at the appropriate points.
AC_DEFUN([SIM_AC_OUTPUT],
[dnl
AC_CONFIG_FILES(Makefile.sim:Makefile.in)
AC_CONFIG_FILES(Make-common.sim:../common/Make-common.in)
AC_CONFIG_COMMANDS([Makefile],
[echo "Merging Makefile.sim+Make-common.sim into Makefile ..."
 rm -f Makesim1.tmp Makesim2.tmp Makefile
 sed -n -e '/^## COMMON_PRE_/,/^## End COMMON_PRE_/ p' <Make-common.sim >Makesim1.tmp
 sed -n -e '/^## COMMON_POST_/,/^## End COMMON_POST_/ p' <Make-common.sim >Makesim2.tmp
 sed -e '/^## COMMON_PRE_/ r Makesim1.tmp' \
	-e '/^## COMMON_POST_/ r Makesim2.tmp' \
	<Makefile.sim >Makefile
 rm -f Makefile.sim Make-common.sim Makesim1.tmp Makesim2.tmp
])
AC_CONFIG_COMMANDS([stamp-h], [echo > stamp-h])

dnl These are unfortunate.  They are conditionally called by other sim macros
dnl but always used by common/Make-common.in.  So we have to subst here even
dnl when the rest of the code is in the respective macros.
AC_SUBST(sim_bitsize)
AC_SUBST(sim_float)

dnl Used by common/Make-common.in to see which configure script created it.
SIM_COMMON_BUILD_TRUE='#'
SIM_COMMON_BUILD_FALSE=
AC_SUBST(SIM_COMMON_BUILD_TRUE)
AC_SUBST(SIM_COMMON_BUILD_FALSE)

AC_OUTPUT
])
