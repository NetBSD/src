/* Header: util.c,v 7.0.1.2 86/10/20 12:07:46 lwall Exp */

/* Log:	util.c,v
 * Revision 7.0.1.2  86/10/20  12:07:46  lwall
 * Made all exits reset tty.
 *
 * Revision 7.0.1.1  86/10/16  10:54:02  lwall
 * Added Damage.  Fixed random bugs.
 *
 * Revision 7.0  86/10/08  15:14:31  lwall
 * Split into separate files.  Added amoebas and pirates.
 *
 */

#include "EXTERN.h"
#include "warp.h"
#include "sys/dir.h"
#include "object.h"
#include "sig.h"
#include "term.h"
#include "INTERN.h"
#include "util.h"

struct timespec timebuf;

void
util_init(void)
{
    ;
}

void
movc3(int len, char *src, char *dest)
{
    if (dest <= src) {
	for (; len; len--) {
	    *dest++ = *src++;
	}
    }
    else {
	dest += len;
	src += len;
	for (; len; len--) {
	    *--dest = *--src;
	}
    }
}

void
no_can_do(const char *what)
{
    fprintf(stderr,"Sorry, your terminal is too %s to play warp.\r\n",what);
    finalize(1);
}

int
exdis(int maxnum)
{
    double temp, temp2;

    temp = (double) maxnum;
#ifndef lint
    temp2 = (double) myrand();
#else
    temp2 = 0.0;
#endif
#if RANDBITS == 15
    return (int) exp(temp2 * log(temp)/0x7fff);
#else
#if RANDBITS == 16
    return (int) exp(temp2 * log(temp)/0xffff);
#else
    return (int) exp(temp2 * log(temp)/0x7fffffff);
#endif
#endif
}

static char nomem[] = "warp: out of memory!\r\n";

/* paranoid version of malloc */

void *
safemalloc(size_t size)
{
    char *ptr;

    ptr = malloc(size?size:1);	/* malloc(0) is NASTY on our system */
    if (ptr != NULL)
	return ptr;
    else {
	fputs(nomem,stdout);
	sig_catcher(0);
    }
    /*NOTREACHED*/
    return NULL;
}

/* safe version of string copy */

char *
safecpy(char *to, const char *from, size_t len)
{
    char *dest = to;

    if (from != NULL)
	for (len--; len && (*dest++ = *from++); len--)
	    continue;
    *dest = '\0';
    return to;
}

/* copy a string up to some (non-backslashed) delimiter, if any */

char *
cpytill(char *to, const char *from, int delim)
{
    for (; *from; from++,to++) {
	if (*from == '\\' && from[1] == delim)
	    from++;
	else if (*from == delim)
	    break;
	*to = *from;
    }
    *to = '\0';
    return __UNCONST(from);
}

/* return ptr to little string in big string, NULL if not found */

char *
instr(const char *big, const char *little)

{
    const char *t;
    const char *s;
    const char *x;

    for (t = big; *t; t++) {
	for (x=t,s=little; *s; x++,s++) {
	    if (!*x)
		return NULL;
	    if (*s != *x)
		break;
	}
	if (!*s)
	    return __UNCONST(t);
    }
    return NULL;
}

/* effective access */

#ifdef SETUIDGID
int
eaccess(const char *filename, mode_t mod)
{
    mode_t protection;
    uid_t euid;

    mod &= 7;				/* remove extraneous garbage */
    if (stat(filename, &filestat) < 0)
	return -1;
    euid = geteuid();
    if (euid == ROOTID)
	return 0;
    protection = 7 & (filestat.st_mode >>
      (filestat.st_uid == euid ? 6 :
        (filestat.st_gid == getegid() ? 3 : 0)
      ));
    if ((mod & protection) == mod)
	return 0;
    errno = EACCES;
    return -1;
}
#endif

void
prexit(const char *cp)
{
	write(2, cp, strlen(cp));
	sig_catcher(0);
}

/* copy a string to a safe spot */

char *
savestr(const char *str)
{
    char *newaddr = safemalloc((size_t)(strlen(str)+1));

    strcpy(newaddr, str);
    return newaddr;
}

char *
getval(const char *nam, const char *def)
{
    const char *val;

    if ((val = getenv(nam)) == NULL || !*val)
	val = def;
    return __UNCONST(val);
}
