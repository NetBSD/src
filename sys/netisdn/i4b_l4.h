/*
 * Copyright (c) 1997, 2000 Hellmuth Michaelis. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	i4b_l4.h - kernel interface to userland header file
 *	---------------------------------------------------
 *
 *	$Id: i4b_l4.h,v 1.1.1.1.6.1 2002/06/23 17:51:29 jdolecek Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Thu Apr 27 13:28:01 2000]
 *
 *---------------------------------------------------------------------------*/

#ifndef _I4B_L4_H_
#define _I4B_L4_H_

extern void i4bputqueue ( struct mbuf *m );
extern void i4bputqueue_hipri(struct mbuf *m);
extern void i4b_l4_accounting(int, int, int, int, int, int, int, int);
extern void i4b_l4_alert_ind ( call_desc_t *cd );
extern void i4b_l4_charging_ind( call_desc_t *cd );
extern void i4b_l4_connect_active_ind ( call_desc_t *cd );
extern void i4b_l4_connect_ind ( call_desc_t *cd );
extern void i4b_l4_daemon_attached(void);
extern void i4b_l4_daemon_detached(void);
extern void i4b_l4_dialout( int driver, int driver_unit );
extern void i4b_l4_dialoutnumber(int driver, int driver_unit, int cmdlen, char *cmd);
extern void i4b_l4_disconnect_ind ( call_desc_t *cd );
extern void i4b_l4_drvrdisc (int cdid);
extern void i4b_l4_negcomplete( call_desc_t *cd );
extern void i4b_l4_ifstate_changed( call_desc_t *cd, int new_state );
extern void i4b_l4_idle_timeout_ind( call_desc_t *cd );
extern void i4b_l4_info_ind ( call_desc_t *cd );
extern void i4b_l4_packet_ind(int, int, int, struct mbuf *pkt);
extern void i4b_l4_l12stat(struct isdn_l3_driver *, int layer, int state);
extern void i4b_l4_pdeact(struct isdn_l3_driver *, int numactive);
extern void i4b_l4_teiasg(struct isdn_l3_driver *, int tei);
extern void i4b_l4_status_ind ( call_desc_t *cd );
extern void i4b_l4_proceeding_ind ( call_desc_t *cd );
extern void i4b_idle_check(call_desc_t *cdp);
extern call_desc_t * cd_by_cdid ( unsigned int cdid );
extern void freecd_by_cd ( call_desc_t *cd );
call_desc_t *cd_by_bricr(int bri, int cr, int crf);
void free_all_cd_of_bri(int bri);
extern unsigned char get_rand_cr ( int unit );
extern call_desc_t * reserve_cd ( void );
extern void T400_start ( call_desc_t *cd );
extern void T400_stop ( call_desc_t *cd );
extern void update_controller_leds(struct isdn_l3_driver *d);

#endif /* _I4B_L4_H_ */
