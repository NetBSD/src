#ifndef _DEV_IC_HFA3861A_H_
#define _DEV_IC_HFA3861A_H_

/* Register set for the Intersil HFA3861A. */

#define	HFA3861A_CR49	0x62	/* Read-only register mux control */
#define	HFA3861A_CR49_SEL	__BIT(7)	/* 0: read-only register set 'b'
						 * 1: read-only register set 'a'
						 */
#define	HFA3861A_CR49_RSVD	__BITS(6, 0)

#define	HFA3861A_CR61	0x7c	/* Rx status, read-only, sets 'a' & 'b' */

#define	HFA3861A_CR62	0x7e	/* RSSI, read-only
#define	HFA3861A_CR62_RSSI	__BITS(7, 0)	/* RSSI, sets 'a' & 'b' */

#define	HFA3861A_CR63	0x80	/* Rx status, read-only */
#define	HFA3861A_CR63_SIGNAL	__BITS(7, 6)	/* signal field value,
						 * sets 'a' & 'b' */
/* 1 Mbps */
#define	HFA3861A_CR63_SIGNAL_1MBPS	__SHIFTIN(0, HFA3861A_CR63_SIGNAL)
/* 2 Mbps */
#define	HFA3861A_CR63_SIGNAL_2MBPS	__SHIFTIN(2, HFA3861A_CR63_SIGNAL)
/* 5.5 or 11 Mbps */
#define	HFA3861A_CR63_SIGNAL_OTHER_MBPS	__SHIFTIN(1, HFA3861A_CR63_SIGNAL)
#define	HFA3861A_CR63_SFD	__BIT(5)	/* SFD found, sets 'a' & 'b' */
#define	HFA3861A_CR63_SHORTPRE	__BIT(4)	/* short preamble detected,
						 * sets 'a' & 'b'
						 */
#define	HFA3861A_CR63_SIGNAL_OK	__BIT(3)	/* valid signal field,
						 * sets 'a' & 'b'
						 */
#define	HFA3861A_CR63_CRC16_OK	__BIT(2)	/* valid CRC 16,
						 * sets 'a' & 'b'
						 */
#define	HFA3861A_CR63_ANTENNA	__BIT(1)	/* antenna used,
						 * sets 'a' & 'b'
						 */
#define	HFA3861A_CR63_RSVD	__BIT(0)	/* reserved, sets 'a' & 'b' */

#endif /* _DEV_IC_HFA3861A_H_ */
