/*	$NetBSD: supextern.h,v 1.24 2013/03/08 20:56:44 christos Exp $	*/

struct stat;

/* atoo.c */
unsigned int atoo(char *);

/* expand.c */
int expand(char *, char **, int);

/* ffilecopy.c */
ssize_t ffilecopy(FILE *, FILE *);

/* filecopy.c */
ssize_t filecopy(int, int );

/* log.c */
void logopen(char *);
void logquit(int, const char *, ...)
	__attribute__((__format__(__printf__, 2, 3)));
void logerr(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2))) ;
void loginfo(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2)));
#ifdef LIBWRAP
void logdeny(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2)));
void logallow(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2)));
#endif

/* netcryptvoid.c */
int netcrypt(char *);
int getcryptbuf(int);
void decode(char *, char *, int);
void encode(char *, char *, int);

/* nxtarg.c */
char *nxtarg(char **, const char *);

/* path.c */
void path(const char *, char *, char *);

/* quit.c */
void quit(int, const char *, ...)
	__attribute__((__format__(__printf__, 2, 3)));

/* read_line.c */
char *read_line(FILE *, size_t *, size_t *, const char[3], int);

/* run.c */
int run(char *, ...);
int runv(char *, char **);
int runp(char *, ...);
int runvp(char *, char **);
int runio(char *const[], const char *, const char *, const char *);
int runiofd(char *const[], const int, const int, const int);

/* estrdup.c */
char *estrdup(const char *);

/* scan.c */
int getrelease(char *);
void makescanlists(void);
void getscanlists(void);
void cdprefix(char *);

/* scm.c */
int servicesetup(char *, int);
int service(void);
int serviceprep(void);
int servicekill(void);
int serviceend(void);
int dobackoff(int *, int *);
int request(char *, char *, int *);
int requestend(void);
const char *remotehost(void);
int thishost(char *);
int samehost(void);
int matchhost(char *);
int scmerr(int, const char *, ...)
	__attribute__((__format__(__printf__, 2, 3)));
int byteswap(int);

/* scmio.c */
int writemsg(int);
int writemend(void);
int writeint(int);
int writestring(char *);
int writefile(int);
int writemnull(int);
int writemint(int, int );
int writemstr(int, char *);
int prereadcount(int *);
int readflush(void);
int readmsg(int);
int readmend(void);
int readskip(void);
int readint(int *);
int readstring(char **);
int readfile(int);
int readmnull(int);
int readmint(int, int *);
int readmstr(int, char **);
void crosspatch(void);

/* skipto.c */
char *skipto(const char *, const char *);
char *skipover(const char *, const char *);

/* stree.c */
void Tfree(TREE **);
TREE *Tinsert(TREE **, const char *, int);
TREE *Tsearch(TREE *, const char *);
TREE *Tlookup(TREE *, const char *);
int Trprocess(TREE *, int (*)(TREE *, void *), void *);
int Tprocess(TREE *, int (*)(TREE *, void *), void *);
void Tprint(TREE *, char *);

/* supcmeat.c */
int getonehost(TREE *, void *);
TREE *getcollhost(int *, int *, long *, int *);
void getcoll(void);
int signon(TREE *, int, int *);
int setup(TREE *);
void suplogin(void);
void listfiles(void);
void recvfiles(void);
int prepare(char *, int, int *, struct stat *);
int recvdir(TREE *, int, struct stat *);
int recvsym(TREE *, int, struct stat *);
int recvreg(TREE *, int, struct stat *);
int copyfile(char *, char *);
void finishup(int);
void done(int, const char *, ...)
	__attribute__((__format__(__printf__, 2, 3)));
void goaway(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2)));

/* supcmisc.c */
void prtime(void);
int establishdir(char *);
int makedir(char *, unsigned int, struct stat *);
int estabd(char *, char *);
void ugconvert(char *, char *, int *, int *, int *);
void notify(int, const char *, ...)
	__attribute__((__format__(__printf__, 2, 3)));
void lockout(int);
char *fmttime(time_t);

/* supcname.c */
void getnams(void);

/* supcparse.c */
int parsecoll(COLLECTION *, char *, char *);
time_t getwhen(char *, char *);
int putwhen(char *, time_t);

/* supmsg.c */
int msgsignon(void);
int msgsignonack(void);
int msgsetup(void);
int msgsetupack(void);
int msgcrypt(void);
int msgcryptok(void);
int msglogin(void);
int msglogack(void);
int msgrefuse(void);
int msglist(void);
int msgneed(void);
int msgdeny(void);
int msgsend(void);
int msgrecv(int (*)(TREE *, va_list), ...);
int msgdone(void);
int msggoaway(void);
int msgxpatch(void);
int msgcompress(void);

/* vprintf.c */
/* XXX already in system headers included already - but with different
   argument declarations! */
#if 0
int vprintf(const char *, va_list);
int vfprintf(FILE *, const char *, va_list);
int vsprintf(char *, const char *, va_list);
int vsnprintf(char *, size_t, const char *, va_list);
#endif
