/*	$NetBSD: dp8390.h,v 1.1 2012/10/12 20:15:52 tsutsui Exp $	*/
/*	Id: dp8390.h,v 1.7 2011/10/05 13:16:20 isaki Exp 	*/

/*
 * This file is derived from sys/arch/i386/stand/lib/netif/dp8390.h
 * NetBSD: dp8390.h,v 1.6 2008/12/14 18:46:33 christos Exp
 */

int dp8390_config(void);
void dp8390_stop(void);

extern int dp8390_iobase;
extern int dp8390_membase;
extern int dp8390_memsize;
#ifdef SUPPORT_WD80X3
#ifdef SUPPORT_SMC_ULTRA
extern int dp8390_is790;
#else
#define dp8390_is790 0
#endif
#else
#ifdef SUPPORT_SMC_ULTRA
#define dp8390_is790 1
#endif
#endif

#define dp8390_is790 0
#define IFNAME "ne"
#define RX_BUFBASE 0
#define TX_PAGE_START (dp8390_membase >> ED_PAGE_SHIFT)

extern uint8_t dp8390_cr_proto; /* values always set in CR */
extern uint8_t dp8390_dcr_reg; /* override DCR if LS is set */

int  EtherSend(char *, int);
int  EtherReceive(char *, int);
