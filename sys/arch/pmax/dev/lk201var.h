/*	$NetBSD: lk201var.h,v 1.4 1999/03/22 03:25:29 ad Exp $	*/

#ifndef _LK201VAR_H_
#define _LK201VAR_H_


#ifdef _KERNEL

extern char *kbdMapChar __P((int));
extern void KBDReset __P((dev_t, void (*)(dev_t, int)));
extern void MouseInit __P((dev_t, void (*)(dev_t, int), int (*)(dev_t)));
extern void mouseInput __P((int cc));

extern int LKgetc __P((dev_t dev));
extern void lk_divert __P((int (*getfn) __P ((dev_t dev)), dev_t in_dev));
extern void lk_bell __P ((int ring));
#endif
#endif	/* _LK201VAR_H_ */
