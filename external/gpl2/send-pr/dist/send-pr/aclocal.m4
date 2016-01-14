define([AC_FIND_PROGRAM],dnl
[if test x$3 = x; then _PATH=$PATH; else _PATH=$3; fi
if test -z "[$]$1"; then
  # Extract the first word of `$2', so it can be a program name with args.
  set dummy $2; word=[$]2
  echo checking for $word
  IFS="${IFS= 	}"; saveifs="$IFS"; IFS="${IFS}:"
  for dir in $_PATH; do
    test -z "$dir" && dir=.
    if test -f $dir/$word; then
      $1=$dir/$word
      break
    fi
  done
  IFS="$saveifs"
fi
test -n "[$]$1" && test -n "$verbose" && echo "	setting $1 to [$]$1"
AC_SUBST($1)dnl
])dnl
dnl
define([AC_ECHON],dnl
[echo checking for echo -n
if test "`echo -n foo`" = foo ; then
  ECHON=bsd
  test -n "$verbose" && echo '	using echo -n'
elif test "`echo 'foo\c'`" = foo ; then
  ECHON=sysv
  test -n "$verbose" && echo '	using echo ...\\c'
else
  ECHON=none
  test -n "$verbose" && echo '	using plain echo'
fi])dnl
