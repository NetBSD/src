/* compiled from: . */
/* ../../../../sys/ucred.h:54:OC */ extern struct ucred *crget (/* ??? */);
/* ../../../../sys/ucred.h:55:OC */ extern struct ucred *crcopy (/* ??? */);
/* ../../../../sys/ucred.h:56:OC */ extern struct ucred *crdup (/* ??? */);
/* ../../../../sys/signal.h:102:NC */ extern int sigaddset (sigset_t *, int);
/* ../../../../sys/signal.h:103:NC */ extern int sigdelset (sigset_t *, int);
/* ../../../../sys/signal.h:104:NC */ extern int sigemptyset (sigset_t *);
/* ../../../../sys/signal.h:105:NC */ extern int sigfillset (sigset_t *);
/* ../../../../sys/signal.h:106:NC */ extern int sigismember (const sigset_t *, int);
/* ./machine/param.h:182:OC */ extern void DELAY (/* ??? */);
/* ./machine/endian.h:51:NC */ extern long unsigned int htonl (long unsigned int);
/* ./machine/endian.h:52:NC */ extern short unsigned int htons (short unsigned int);
/* ./machine/endian.h:53:NC */ extern long unsigned int ntohl (long unsigned int);
/* ./machine/endian.h:54:NC */ extern short unsigned int ntohs (short unsigned int);
/* ../../../../sys/select.h:44:NC */ extern void selrecord (struct proc *, struct selinfo *);
/* ../../../../sys/select.h:47:NC */ extern void selwakeup (struct selinfo *);
/* ../../../../sys/proc.h:232:OC */ extern struct proc *pfind (/* ??? */);
/* ../../../../sys/proc.h:234:OC */ extern struct pgrp *pgfind (/* ??? */);
/* ../../../../sys/systm.h:82:NC */ extern int nullop (void);
/* ../../../../sys/systm.h:83:NC */ extern int enodev (void);
/* ../../../../sys/systm.h:84:NC */ extern int enoioctl (void);
/* ../../../../sys/systm.h:85:NC */ extern int enxio (void);
/* ../../../../sys/systm.h:86:NC */ extern int eopnotsupp (void);
/* ../../../../sys/systm.h:88:NC */ extern int selscan (struct proc *, fd_set *, fd_set *, int, int *);
/* ../../../../sys/systm.h:89:NC */ extern int seltrue (dev_t, int, struct proc *);
/* ../../../../sys/systm.h:95:NC */ extern void panic (char *);
/* ../../../../sys/systm.h:96:NC */ extern void tablefull (char *);
/* ../../../../sys/systm.h:97:NC */ extern void addlog (const char *, ...);
/* ../../../../sys/systm.h:98:NC */ extern void log (int, const char *, ...);
/* ../../../../sys/systm.h:99:NC */ extern void printf (const char *, ...);
/* ../../../../sys/systm.h:100:NC */ extern int sprintf (char *, const char *, ...);
/* ../../../../sys/systm.h:101:NC */ extern void ttyprintf (struct tty *, const char *, ...);
/* ../../../../sys/systm.h:103:NC */ extern void bcopy (void *, void *, u_int);
/* ../../../../sys/systm.h:104:NC */ extern void ovbcopy (void *, void *, u_int);
/* ../../../../sys/systm.h:105:NC */ extern void bzero (void *, u_int);
/* ../../../../sys/systm.h:106:NC */ extern int bcmp (void *, void *, u_int);
/* ../../../../sys/systm.h:107:NC */ extern int strlen (const char *);
/* ../../../../sys/systm.h:109:NC */ extern int copystr (void *, void *, u_int, u_int *);
/* ../../../../sys/systm.h:110:NC */ extern int copyinstr (void *, void *, u_int, u_int *);
/* ../../../../sys/systm.h:111:NC */ extern int copyoutstr (void *, void *, u_int, u_int *);
/* ../../../../sys/systm.h:112:NC */ extern int copyin (void *, void *, u_int);
/* ../../../../sys/systm.h:113:NC */ extern int copyout (void *, void *, u_int);
/* ../../../../sys/systm.h:115:NC */ extern int fubyte (void *);
/* ../../../../sys/systm.h:119:NC */ extern int subyte (void *, int);
/* ../../../../sys/systm.h:120:NC */ extern int suibyte (void *, int);
/* ../../../../sys/systm.h:121:NC */ extern int fuword (void *);
/* ../../../../sys/systm.h:122:NC */ extern int fuiword (void *);
/* ../../../../sys/systm.h:123:NC */ extern int suword (void *, int);
/* ../../../../sys/systm.h:124:NC */ extern int suiword (void *, int);
/* ../../../../sys/systm.h:126:NC */ extern int scanc (unsigned int, u_char *, u_char *, int);
/* ../../../../sys/systm.h:127:NC */ extern int skpc (int, int, char *);
/* ../../../../sys/systm.h:128:NC */ extern int locc (int, char *, unsigned int);
/* ../../../../sys/systm.h:129:NC */ extern int ffs (long int);
/* ../../../../sys/malloc.h:270:NC */ extern void *malloc (long unsigned int, int, int);
/* ../../../../sys/malloc.h:271:NC */ extern void free (void *, int);
/* ../../../../arch/amiga/dev/ite.c:74:OC */ extern int nodev (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:76:OC */ extern int customc_scroll (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:76:OC */ extern int customc_init (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:76:OC */ extern int customc_deinit (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:77:OC */ extern int customc_clear (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:77:OC */ extern int customc_putc (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:77:OC */ extern int customc_cursor (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:79:OC */ extern int tiga_scroll (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:79:OC */ extern int tiga_init (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:79:OC */ extern int tiga_deinit (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:80:OC */ extern int tiga_clear (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:80:OC */ extern int tiga_putc (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:80:OC */ extern int tiga_cursor (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:106:OC */ extern int itestart (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:107:OC */ extern int ttrstrt (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:125:OF */ extern int iteon (dev_t dev, int flag); /* (dev, flag) dev_t dev; int flag; */
/* ../../../../arch/amiga/dev/ite.c:149:IC */ extern int kbdenable (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:151:IC */ extern int iteinit (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:156:OF */ extern int iteinit (dev_t dev); /* (dev) dev_t dev; */
/* ../../../../arch/amiga/dev/ite.c:197:OF */ extern int iteoff (dev_t dev, int flag); /* (dev, flag) dev_t dev; int flag; */
/* ../../../../arch/amiga/dev/ite.c:221:NF */ extern int iteopen (dev_t dev, int mode, int devtype, struct proc *p); /* (dev, mode, devtype, p) dev_t dev; int mode; int devtype; struct proc *p; */
/* ../../../../arch/amiga/dev/ite.c:248:IC */ extern int ttychars (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:257:IC */ extern int ttsetwater (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:270:OF */ extern int iteclose (dev_t dev, int flag, int mode, struct proc *p); /* (dev, flag, mode, p) dev_t dev; int flag; int mode; struct proc *p; */
/* ../../../../arch/amiga/dev/ite.c:277:IC */ extern int ttyclose (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:288:OF */ extern int iteread (dev_t dev, struct uio *uio, int flag); /* (dev, uio, flag) dev_t dev; struct uio *uio; int flag; */
/* ../../../../arch/amiga/dev/ite.c:299:OF */ extern int itewrite (dev_t dev, struct uio *uio, int flag); /* (dev, uio, flag) dev_t dev; struct uio *uio; int flag; */
/* ../../../../arch/amiga/dev/ite.c:312:OF */ extern int iteioctl (dev_t dev, int cmd, caddr_t addr, int flag); /* (dev, cmd, addr, flag) dev_t dev; int cmd; caddr_t addr; int flag; */
/* ../../../../arch/amiga/dev/ite.c:321:IC */ extern int ttioctl (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:328:OF */ extern int itestart (register struct tty *tp); /* (tp) register struct tty *tp; */
/* ../../../../arch/amiga/dev/ite.c:335:IC */ extern int spl0 (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:343:IC */ extern int wakeup (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:358:IC */ extern int getc (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:368:IC */ extern int spl0 (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:369:IC */ extern int iteputchar (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:374:IC */ extern int timeout (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:377:IC */ extern int spl0 (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:386:OF */ static void repeat_handler (int a0, int a1); /* (a0, a1) int a0; int a1; */
/* ../../../../arch/amiga/dev/ite.c:391:IC */ extern int itefilter (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:397:OF */ extern int itefilter (register u_char c, enum caller caller); /* (c, caller) register u_char c; enum caller caller; */
/* ../../../../arch/amiga/dev/ite.c:475:IC */ extern int untimeout (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:524:IC */ extern int timeout (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:530:IC */ extern int timeout (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:589:OF */ static void ite_dchar (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:602:OF */ static void ite_ichar (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:615:OF */ static void ite_clrtoeol (struct ite_softc *ip, struct itesw *sp, int y, int x); /* (ip, sp, y, x) struct ite_softc *ip; struct itesw *sp; int y; int x; */
/* ../../../../arch/amiga/dev/ite.c:626:OF */ static void ite_clrtobol (struct ite_softc *ip, struct itesw *sp, int y, int x); /* (ip, sp, y, x) struct ite_softc *ip; struct itesw *sp; int y; int x; */
/* ../../../../arch/amiga/dev/ite.c:637:OF */ static void ite_clrline (struct ite_softc *ip, struct itesw *sp, int y, int x); /* (ip, sp, y, x) struct ite_softc *ip; struct itesw *sp; int y; int x; */
/* ../../../../arch/amiga/dev/ite.c:650:OF */ static void ite_clrtoeos (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:660:OF */ static void ite_clrtobos (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:670:OF */ static void ite_clrscreen (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:682:OF */ static void ite_dline (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:693:OF */ static void ite_iline (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:704:OF */ static void ite_lf (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:720:OF */ static void ite_crlf (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:729:OF */ static void ite_cr (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:741:OF */ static void ite_rlf (struct ite_softc *ip, struct itesw *sp); /* (ip, sp) struct ite_softc *ip; struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:756:OF */ static int atoi (const char *cp); /* (cp) const char *cp; */
/* ../../../../arch/amiga/dev/ite.c:768:OF */ static char *index (const char *cp, char ch); /* (cp, ch) const char *cp; char ch; */
/* ../../../../arch/amiga/dev/ite.c:779:OF */ static int ite_argnum (struct ite_softc *ip); /* (ip) struct ite_softc *ip; */
/* ../../../../arch/amiga/dev/ite.c:797:OF */ static int ite_zargnum (struct ite_softc *ip); /* (ip) struct ite_softc *ip; */
/* ../../../../arch/amiga/dev/ite.c:815:OF */ static void ite_sendstr (struct ite_softc *ip, char *str); /* (ip, str) struct ite_softc *ip; char *str; */
/* ../../../../arch/amiga/dev/ite.c:824:OF */ static int strncmp (const char *a, const char *b, int l); /* (a, b, l) const char *a; const char *b; int l; */
/* ../../../../arch/amiga/dev/ite.c:835:OF */ extern int iteputchar (register int c, dev_t dev); /* (c, dev) register int c; dev_t dev; */
/* ../../../../arch/amiga/dev/ite.c:1495:IC */ extern int itecheckwrap (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:1500:IC */ extern int kbdbell (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:1597:OF */ extern int itecheckwrap (register struct ite_softc *ip, register struct itesw *sp); /* (ip, sp) register struct ite_softc *ip; register struct itesw *sp; */
/* ../../../../arch/amiga/dev/ite.c:1630:OF */ extern int itecnprobe (struct consdev *cp); /* (cp) struct consdev *cp; */
/* ../../../../arch/amiga/dev/ite.c:1641:IC */ extern int grfconfig (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:1684:OF */ extern int itecninit (struct consdev *cp); /* (cp) struct consdev *cp; */
/* ../../../../arch/amiga/dev/ite.c:1692:IC */ extern int kbdenable (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:1697:OF */ extern int itecngetc (dev_t dev); /* (dev) dev_t dev; */
/* ../../../../arch/amiga/dev/ite.c:1703:IC */ extern int kbdgetcn (/* ??? */);
/* ../../../../arch/amiga/dev/ite.c:1712:OF */ extern int itecnputc (dev_t dev, int c); /* (dev, c) dev_t dev; int c; */
