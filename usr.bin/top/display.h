/*	$NetBSD: display.h,v 1.5 2002/03/23 01:28:10 thorpej Exp $	*/

/* constants needed for display.c */

/* "type" argument for new_message function */

#define  MT_standout  1
#define  MT_delayed   2

int display_resize __P((void));
struct statics;
int display_init __P((struct statics *));
void i_loadave __P((int, double *));
void u_loadave __P((int, double *));
void i_timeofday __P((time_t *));
void i_procstates __P((int, int *));
void u_procstates __P((int, int *));
char *cpustates_tag __P((void));
void i_cpustates __P((int *));
void u_cpustates __P((int *));
void z_cpustates __P((void));
void i_memory __P((int *));
void u_memory __P((int *));
void i_swap __P((int *));
void u_swap __P((int *));
void i_message __P((void));
void u_message __P((void));
void i_header __P((char *));
void u_header __P((char *));
void i_process __P((int, char *));
void u_process __P((int, char *));
void u_endscreen __P((int));
void display_header __P((int));
void new_message __P((int, const char *, ...))
     __attribute__((__format__(__printf__, 2, 3)));
void clear_message __P((void));
int readline __P((char *, int, int));
char *printable __P((char *));
