ino_t createfileondev __P((char *, char *, char *, int));
void cleanupfileondev __P((char *, char *, int));

char *getmountpoint __P((char *));
void cleanupmount __P((char *));

extern int verbose;
