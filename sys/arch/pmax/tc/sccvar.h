/*	$NetBSD: sccvar.h,v 1.7 2000/02/03 04:09:11 nisimura Exp $	*/

#ifndef _PMAX_TC_SCCVAR_H_
#define _PMAX_TC_SCCVAR_H_

/*
 * Minor device numbers for scc. Weird because B channel comes
 * first and the A channels are wired for keyboard/mouse and the
 * B channels for the comm port(s).
 *
 * XXX
 *
 * Even that is not true on the Personal Decstation, which has
 * a "desktop bus" for keyboard/mouse, and brings A and B channels
 * out to the bulkhead.  XXX more thought.
 */

#define	SCCCOMM2_PORT	0x0
#define	SCCMOUSE_PORT	0x1
#define	SCCCOMM3_PORT	0x2
#define	SCCKBD_PORT	0x3

int	sccGetc __P((dev_t));
void	sccPutc __P((dev_t, int));

/* QVSS-compatible in-kernel X input event parser, pointer tracker */
extern void	(*sccDivertXInput) __P((int));
extern void	(*sccMouseEvent) __P((void *));
extern void	(*sccMouseButtons) __P((void *));

void scc_cnattach __P((u_int32_t, u_int32_t));
void scc_lk201_cnattach __P((u_int32_t, u_int32_t));

#endif	/* !_PMAX_TC_SCCVAR_H_ */
