#ifdef HAS_MYSQL

#include <time.h>
#include "mysql.h"

#define STATACTIVE 0
#define STATFAIL 1
#define STATUNTRIED 2
#define RETRY_CONN_INTV 60		/* 1 minute */

extern DICT *dict_mysql_open(const char *name, int unused_flags, int dict_flags);


#endif
