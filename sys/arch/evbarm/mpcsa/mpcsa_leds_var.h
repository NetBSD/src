/*	$NetBSD: mpcsa_leds_var.h,v 1.1.16.1 2008/09/28 10:39:54 mjf Exp $	*/

#ifndef	_MPCSA_LEDS_VAR_H_
#define	_MPCSA_LEDS_VAR_H_

#define	INFINITE_BLINK	8191

void mpcsa_blink_led(int num, int interval);
void mpcsa_comm_led(int num, int count);
void mpcsa_conn_led(int num, int ok);

#endif	// !_MPCSA_LEDS_VAR_H_
