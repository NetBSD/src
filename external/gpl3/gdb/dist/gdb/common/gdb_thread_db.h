#ifdef HAVE_THREAD_DB_H
#include <thread_db.h>
#else
#include "glibc_thread_db.h"
#endif

#ifndef LIBTHREAD_DB_SO
#define LIBTHREAD_DB_SO "libthread_db.so.1"
#endif

#ifndef LIBTHREAD_DB_SEARCH_PATH
/* $sdir appears before $pdir for some minimal security protection:
   we trust the system libthread_db.so a bit more than some random
   libthread_db associated with whatever libpthread the app is using.  */
#define LIBTHREAD_DB_SEARCH_PATH "$sdir:$pdir"
#endif
