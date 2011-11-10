
#ifdef TRE_USE_ALLOCA
/* AIX requires this to be the first thing in the file.	 */
#if !defined(__GNUC__) && !defined(__lint__)
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif
#endif /* TRE_USE_ALLOCA */
