/*	$NetBSD: lk201var.h,v 1.8 2000/01/08 01:02:35 simonb Exp $	*/

#ifndef _LK201VAR_H_
#define _LK201VAR_H_

#ifdef _KERNEL

char	*lk_mapchar __P((int, int *));
void	 lk_reset __P((dev_t, void (*)(dev_t, int)));
void	 lk_mouseinit __P((dev_t, void (*)(dev_t, int), int (*)(dev_t)));

int	 lk_getc __P((dev_t dev));
void	 lk_divert __P((int (*getfn) __P((dev_t dev)), dev_t in_dev));
void	 lk_bell __P((int ring));

#endif
#endif	/* _LK201VAR_H_ */
