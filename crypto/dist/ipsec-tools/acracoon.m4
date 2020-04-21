dnl RACOON_PATH_LIBS(FUNCTION, LIB, SEARCH-PATHS [, ACTION-IF-FOUND
dnl            [, ACTION-IF-NOT-FOUND [, OTHER-LIBRARIES]]])
dnl Search for a library defining FUNC, if it's not already available.

AC_DEFUN([RACOON_PATH_LIBS],
[AC_PREREQ([2.13])
AC_CACHE_CHECK([for $2 containing $1], [ac_cv_search_$1],
[ac_func_search_save_LIBS="$LIBS"
ac_cv_search_$1="no"
AC_TRY_LINK_FUNC([$1], [ac_cv_search_$1="none required"],
	[LIBS="-l$2 $LIBS"
	AC_TRY_LINK_FUNC([$1], [ac_cv_search_$1="-l$2"], [])])
LIBS="$ac_func_search_save_LIBS"
ifelse("x$3", "x", , [ test "$ac_cv_search_$1" = "no" && for i in $3; do
  LIBS="-L$i -l$2 $ac_func_search_save_LIBS"
  AC_TRY_LINK_FUNC([$1],
  [ac_cv_search_$1="-L$i -l$2"
  break])
  done 
LIBS="$ac_func_search_save_LIBS" ]) ])
if test "$ac_cv_search_$1" != "no"; then
  test "$ac_cv_search_$1" = "none required" || LIBS="$ac_cv_search_$1 $LIBS"
  $4
else :
  $5
fi])

dnl  Check if either va_copy() or __va_copy() is available. On linux systems 
dnl  at least one of these should be present.
AC_DEFUN([RACOON_CHECK_VA_COPY], [
	saved_CFLAGS=$CFLAGS
	CFLAGS="-Wall -O2"
	AC_CACHE_CHECK([for an implementation of va_copy()],
		ac_cv_va_copy,[
		AC_TRY_RUN([#include <stdarg.h>
		void func (int i, ...) {
			va_list args1, args2;
			va_start (args1, i);
			va_copy (args2, args1);
			if (va_arg (args1, int) != 1 || va_arg (args2, int) != 1)
				exit (1);
	 		va_end (args1);
			va_end (args2);
		}
		int main() {
			func (0, 1);
			return 0;
		}],
		[ac_cv_va_copy=yes],
		[ac_cv_va_copy=no],
		AC_MSG_WARN(Cross compiling... Unable to test va_copy)
		[ac_cv_va_copy=no])
	])
	if test x$ac_cv_va_copy != xyes; then
		AC_CACHE_CHECK([for an implementation of __va_copy()],
			ac_cv___va_copy,[
			AC_TRY_RUN([#include <stdarg.h>
			void func (int i, ...) {
				va_list args1, args2;
				va_start (args1, i);
				__va_copy (args2, args1);
				if (va_arg (args1, int) != 1 || va_arg (args2, int) != 1)
					exit (1);
				va_end (args1);
				va_end (args2);
			}
			int main() {
				func (0, 1);
				return 0;
			}],
			[ac_cv___va_copy=yes],
			[ac_cv___va_copy=no],
			AC_MSG_WARN(Cross compiling... Unable to test __va_copy)
			[ac_cv___va_copy=no])
		])
	fi

	if test "x$ac_cv_va_copy" = "xyes"; then
		va_copy_func=va_copy
	elif test "x$ac_cv___va_copy" = "xyes"; then
		va_copy_func=__va_copy
	fi

	if test -n "$va_copy_func"; then
		AC_DEFINE_UNQUOTED(VA_COPY,$va_copy_func,
			[A 'va_copy' style function])
	else
		AC_MSG_WARN([Hmm, neither va_copy() nor __va_copy() found.])
		AC_MSG_WARN([Using a generic fallback.])
	fi
	CFLAGS=$saved_CFLAGS
	unset saved_CFLAGS
])

AC_DEFUN([RACOON_CHECK_BUGGY_GETADDRINFO], [
	AC_MSG_CHECKING(getaddrinfo bug)
	saved_CFLAGS=$CFLAGS
	CFLAGS="-Wall -O2"
	AC_TRY_RUN([
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <stdlib.h>
	#include <string.h>
	#include <netinet/in.h>
	
	int main()
	{
	  int passive, gaierr, inet4 = 0, inet6 = 0;
	  struct addrinfo hints, *ai, *aitop;
	  char straddr[INET6_ADDRSTRLEN], strport[16];
	
	  for (passive = 0; passive <= 1; passive++) {
	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_flags = passive ? AI_PASSIVE : 0;
	    hints.ai_protocol = IPPROTO_TCP;
	    hints.ai_socktype = SOCK_STREAM;
	    if ((gaierr = getaddrinfo(NULL, "54321", &hints, &aitop)) != 0) {
	      (void)gai_strerror(gaierr);
	      goto bad;
	    }
	    for (ai = aitop; ai; ai = ai->ai_next) {
	      if (ai->ai_addr == NULL ||
	          ai->ai_addrlen == 0 ||
	          getnameinfo(ai->ai_addr, ai->ai_addrlen,
	                      straddr, sizeof(straddr), strport, sizeof(strport),
	                      NI_NUMERICHOST|NI_NUMERICSERV) != 0) {
	        goto bad;
	      }
	      switch (ai->ai_family) {
	      case AF_INET:
		if (strcmp(strport, "54321") != 0) {
		  goto bad;
		}
	        if (passive) {
	          if (strcmp(straddr, "0.0.0.0") != 0) {
	            goto bad;
	          }
	        } else {
	          if (strcmp(straddr, "127.0.0.1") != 0) {
	            goto bad;
	          }
	        }
	        inet4++;
	        break;
	      case AF_INET6:
		if (strcmp(strport, "54321") != 0) {
		  goto bad;
		}
	        if (passive) {
	          if (strcmp(straddr, "::") != 0) {
	            goto bad;
	          }
	        } else {
	          if (strcmp(straddr, "::1") != 0) {
	            goto bad;
	          }
	        }
	        inet6++;
	        break;
	      case AF_UNSPEC:
	        goto bad;
	        break;
	      default:
	        /* another family support? */
	        break;
	      }
	    }
	  }
	
	  if (!(inet4 == 0 || inet4 == 2))
	    goto bad;
	  if (!(inet6 == 0 || inet6 == 2))
	    goto bad;
	
	  if (aitop)
	    freeaddrinfo(aitop);
	  exit(0);
	
	 bad:
	  if (aitop)
	    freeaddrinfo(aitop);
	  exit(1);
	}
	],
	AC_MSG_RESULT(good)
	buggygetaddrinfo=no,
	AC_MSG_RESULT(buggy)
	buggygetaddrinfo=yes,
	AC_MSG_RESULT(Cross compiling ... Assuming getaddrinfo is not buggy.)
	buggygetaddrinfo=no)
	CFLAGS=$saved_CFLAGS
	unset saved_CFLAGS
])
