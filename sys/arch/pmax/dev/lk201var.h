/*	$NetBSD: lk201var.h,v 1.4.14.1 1999/12/27 18:33:24 wrstuden Exp $	*/

#ifndef _LK201VAR_H_
#define _LK201VAR_H_

#ifdef _KERNEL

char	*kbdMapChar __P((int, int *));
void	KBDReset __P((dev_t, void (*)(dev_t, int)));
void	MouseInit __P((dev_t, void (*)(dev_t, int), int (*)(dev_t)));
void	mouseInput __P((int cc));

int	LKgetc __P((dev_t dev));
void	lk_divert __P((int (*getfn) __P ((dev_t dev)), dev_t in_dev));
void	lk_bell __P ((int ring));

#endif
#endif	/* _LK201VAR_H_ */
