/* Header: util.h,v 7.0 86/10/08 15:14:37 lwall Exp */

/* Log:	util.h,v
 * Revision 7.0  86/10/08  15:14:37  lwall
 * Split into separate files.  Added amoebas and pirates.
 *
 */

#define RANDRAND 1152921504606846976.0 /* that's 2**60 */
#define HALFRAND 0x40000000 /* that's 2**30 */
#define myrand() (int)random()
#define rand_mod(m) ((myrand() / 37) % (m)) /* pick number in 0..m-1 */
/*
 * The reason for the /37 above is that our random number generator yields
 * successive evens and odds, for some reason.  This makes strange star maps.
 */

    /* we get fractions of seconds from calling ftime on timebuf */

extern struct timespec timebuf;
#define roundsleep(x) (clock_gettime(CLOCK_REALTIME, &timebuf),sleep(timebuf.tv_nsec > (500 * 1000 * 1000) ?x+1:x))

#define waiting 0

#ifdef NOTDEF
EXT int len_last_line_got INIT(0);
			/* strlen of some_buf after */
			/*  some_buf = get_a_line(bufptr,buffersize,fp) */
#endif

#ifdef NOTDEF
/* is the string for makedir a directory name or a filename? */

#define MD_DIR 0
#define MD_FILE 1
#endif

void util_init(void);
void movc3(int, char *, char *);
__dead void no_can_do(const char *);
int exdis(int);
void *safemalloc(size_t size);
char *safecpy(char *, const char *, size_t);
char *cpytill(char *, const char *, int);
char *instr(const char *, const char *);
#ifdef SETUIDGID
int eaccess(const char *, mode_t);
#endif
__dead void prexit(const char *);
char *savestr(const char *);
char *getval(const char *, const char *);
