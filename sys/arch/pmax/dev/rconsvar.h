#ifndef _MACHINE_RCONS_H_
#define _MACHINE_RCONS_H_

#ifdef _KERNEL
struct consdev;
extern void rcons_indev __P((struct consdev *));
extern void rcons_vputc __P((dev_t, int));	/* XXX */
extern void rcons_input __P((dev_t, int ic));	/*XXX*/
#endif	/* _KERNEL */
#endif	/* _MACHINE_RCONS_H_ */

