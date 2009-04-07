dnl From Mark D Baushke & Derek Price.
dnl
dnl See if select() on a /dev/null fd hangs when timeout is NULL.
dnl Also check to see that /dev/null is in the readfds set returned.
dnl
dnl Observed on Solaris 7:
dnl   If /dev/null is in the readfds set, it will never be marked as
dnl   ready by the OS. In the case of a /dev/null fd being the only fd
dnl   in the select set and timeout == NULL, the select will hang.
dnl
dnl If the test fails, then arrange to use select only via a wrapper
dnl function that works around the problem.

AC_DEFUN([ccvs_FUNC_SELECT],
[
 AC_CHECK_HEADERS([fcntl.h])
 AC_CACHE_CHECK([whether select hangs on /dev/null fd when timeout is NULL],
  ccvs_cv_func_select_hang,
  [AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <sys/select.h>
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#include <errno.h>]], [[
  int numfds;
  fd_set readfds;
  struct timeval timeout;
  int fd = open ("/dev/null", O_RDONLY);

  FD_ZERO (&readfds);
  FD_SET (fd, &readfds);
  timeout.tv_sec = 0;
  timeout.tv_usec = 1;

  while ((numfds = select (fd + 1, &readfds, NULL, NULL, &timeout)) < 0
	 && errno == EINTR);
  return (numfds <= 0);
	  ]])],
	 ccvs_cv_func_select_hang=no,
	 ccvs_cv_func_select_hang=yes,
	 dnl When crosscompiling, assume it is broken.
	 ccvs_cv_func_select_hang=yes)
  ])
  if test $ccvs_cv_func_select_hang = yes; then
    ccvs_PREREQ_SELECT

    AC_LIBOBJ(sunos57-select)
    AC_DEFINE(select, rpl_select,
      [Define to rpl_select if the replacement function should be used.])
  fi
])

AC_DEFUN([ccvs_PREREQ_SELECT], [
  AC_CHECK_HEADERS(fcntl.h unistd.h)])
