/*	$NetBSD: perl_extern.h,v 1.3 2001/03/31 11:37:51 aymeric Exp $	*/

int perl_end __P((GS *));
int perl_init __P((SCR *));
int perl_screen_end __P((SCR*));
int perl_ex_perl __P((SCR*, CHAR_T *, size_t, recno_t, recno_t));
int perl_ex_perldo __P((SCR*, CHAR_T *, size_t, recno_t, recno_t));
#ifdef USE_SFIO
Sfdisc_t* sfdcnewnvi __P((SCR*));
#endif
