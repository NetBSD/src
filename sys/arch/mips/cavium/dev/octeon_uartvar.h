/*	$NetBSD: octeon_uartvar.h,v 1.1 2020/06/27 02:49:42 simonb Exp $	*/

#ifndef _OCTEON_UARTVAR_H_
#define _OCTEON_UARTVAR_H_

void	octuart_early_cnattach(int);
int	octuart_com_cnattach(bus_space_tag_t, int, int);

#endif /* _OCTEON_UARTVAR_H_ */
