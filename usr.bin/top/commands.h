/*	$NetBSD: commands.h,v 1.2 1999/04/12 06:02:25 ross Exp $	*/

void show_help __P((struct statics *));
char *next_field __P((char *));
int scanint __P((char *, int *));
char *err_string __P((void));
int str_adderr __P((char *, int, int));
int str_addarg __P((char *, int, char *, int));
struct errs;
int err_compar __P((const void *, const void *));
int error_count __P((void));
void show_errors __P((void));
char *kill_procs __P((char *));
char *renice_procs __P((char *));
