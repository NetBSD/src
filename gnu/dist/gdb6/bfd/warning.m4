dnl Common configure.in fragment

AC_DEFUN([AM_BINUTILS_WARNINGS],[
WARN_CFLAGS="-W -Wall -Wstrict-prototypes -Wmissing-prototypes"

AC_ARG_ENABLE(werror,
  [  --enable-werror    treat compile warnings as errors],
  [case "${enableval}" in
     yes | y) ERROR_ON_WARNING="yes" ;;
     no | n)  ERROR_ON_WARNING="no" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --enable-werror) ;;
   esac])

# Enable -Werror by default when using gcc
if test "${GCC}" = yes -a -z "${ERROR_ON_WARNING}" ; then
    ERROR_ON_WARNING=yes
fi

NO_WERROR=
if test "${ERROR_ON_WARNING}" = yes ; then
    WARN_CFLAGS="$WARN_CFLAGS -Werror"
    NO_WERROR="-Wno-error"
fi
		   
AC_ARG_ENABLE(build-warnings,
[  --enable-build-warnings Enable build-time compiler warnings],
[case "${enableval}" in
  yes)	;;
  no)	WARN_CFLAGS="-w";;
  ,*)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        WARN_CFLAGS="${WARN_CFLAGS} ${t}";;
  *,)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        WARN_CFLAGS="${t} ${WARN_CFLAGS}";;
  *)    WARN_CFLAGS=`echo "${enableval}" | sed -e "s/,/ /g"`;;
esac])

if test x"$silent" != x"yes" && test x"$WARN_CFLAGS" != x""; then
  echo "Setting warning flags = $WARN_CFLAGS" 6>&1
fi

AC_SUBST(WARN_CFLAGS)
AC_SUBST(NO_WERROR)
])
