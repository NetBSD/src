/*	$NetBSD: hd64461.h,v 1.2 2001/03/22 18:27:51 uch Exp $	*/

#include "../../../../hpcsh/dev/hd64461/hd64461reg.h"
#include "../../../../hpcsh/dev/hd64461/hd64461intcreg.h"
#include "../../../../hpcsh/dev/hd64461/hd64461pcmciareg.h"
#include "../../../../hpcsh/dev/hd64461/hd64461gpioreg.h"
#include "../../../../hpcsh/dev/hd64461/hd64461uartreg.h"

#define	LSR_TXRDY	0x20	/* Transmitter buffer empty */

#define HD64461COM_TX_BUSY()						\
	while ((VOLATILE_REF8(HD64461_ULSR_REG8) & LSR_TXRDY) == 0)

#define HD64461COM_PUTC(c)						\
__BEGIN_MACRO								\
	HD64461COM_TX_BUSY();						\
	VOLATILE_REF8(HD64461_UTBR_REG8) = (c);				\
	HD64461COM_TX_BUSY();						\
__END_MACRO

#define HD64461COM_PRINT(s)						\
__BEGIN_MACRO								\
	char *__s =(char *)(s);						\
	int __i;							\
	for (__i = 0; __s[__i] != '\0'; __i++) {			\
		char __c = __s[__i];					\
		if (__c == '\n')					\
			HD64461COM_PUTC('\r');				\
		HD64461COM_PUTC(__c);					\
	}								\
__END_MACRO
