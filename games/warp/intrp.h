/* Header: intrp.h,v 7.0.1.1 86/12/12 16:59:45 lwall Exp
 *
 * Log:	intrp.h,v
 * Revision 7.0.1.1  86/12/12  16:59:45  lwall
 * Baseline for net release.
 *
 * Revision 7.0  86/10/08  15:12:27  lwall
 * Split into separate files.  Added amoebas and pirates.
 *
 */

EXT char *origdir INIT(NULL);		/* cwd when warp invoked */
EXT char *homedir INIT(NULL);		/* login directory */
EXT char *dotdir INIT(NULL);		/* where . files go */
EXT char *logname INIT(NULL);		/* login id */
EXT char *hostname;	/* host name */
EXT char *realname INIT(NULL);	/* real name from /etc/passwd */

void intrp_init(char *);
char *filexp(const char *);
void interp(char *, size_t, const char *);
char *dointerp(char *, size_t, const char *, const char *);
