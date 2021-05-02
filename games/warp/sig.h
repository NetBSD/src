/* Header: sig.h,v 7.0 86/10/08 15:13:32 lwall Exp */

/* Log:	sig.h,v
 * Revision 7.0  86/10/08  15:13:32  lwall
 * Split into separate files.  Added amoebas and pirates.
 *
 */

void sig_init(void);
void mytstp(void);
__dead void finalize(int status);
__dead void sig_catcher(int signo);
#ifdef SIGTSTP
void cont_catcher(int x);
void stop_catcher(int sig);
#endif
void sig_catcher(int);
