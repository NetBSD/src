/* Author: Wietse Venema <wietse@wzv.win.tue.nl> */

/* $Id: sysv_default.h,v 1.1.1.1.4.2 2000/06/16 18:46:18 thorpej Exp $ */

extern char *default_console;
extern char *default_altsh;
extern char *default_passreq;
extern char *default_timezone;
extern char *default_hz;
extern char *default_path;
extern char *default_supath;
extern char *default_ulimit;
extern char *default_timeout;
extern char *default_umask;
extern char *default_sleep;
extern char *default_maxtrys;

void sysv_defaults(void);
