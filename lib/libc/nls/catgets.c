/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(__catgets,catgets);
#else

#include <nl_types.h>

extern char * __catgets __P((nl_catd, int, int, char *));

char *
catgets(catd, set_id, msg_id, s)
	nl_catd catd;
	int set_id;
	int msg_id;
	char *s;
{
	return __catgets(catd, set_id, msg_id, s);
}

#endif
