/*	$NetBSD: tx39var.h,v 1.2 1999/12/22 15:35:35 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

struct tx_chipset_tag {
	void *tc_intrt;  /* interrupt tag */
	void *tc_powert; /* power/clock tag */
};

typedef struct tx_chipset_tag* tx_chipset_tag_t;
typedef u_int32_t txreg_t; 

/*
 *	TX39 Internal Function Register access
 */
#define TX39_SYSADDR_CONFIG_REG_KSEG1	0xb0c00000

#ifdef TX39_PREFER_FUNCTION

tx_chipset_tag_t tx_conf_get_tag __P((void));
u_int32_t	 tx_conf_read __P((tx_chipset_tag_t, int));
void		 tx_conf_write __P((tx_chipset_tag_t, int, txreg_t));

#else /* TX39_PREFER_FUNCTION */

extern struct tx_chipset_tag tx_chipset;
#define tx_conf_read(t, reg) ((void)(t), \
	(*((volatile txreg_t*)(TX39_SYSADDR_CONFIG_REG_KSEG1 + (reg)))))
#define tx_conf_write(t, reg, val) ((void)(t), \
	(*((volatile txreg_t*)(TX39_SYSADDR_CONFIG_REG_KSEG1 + (reg))) \
	= (val)))
#define tx_conf_get_tag() (&tx_chipset)

#endif /* TX39_PREFER_FUNCTION */

/*
 *	txsim attach arguments. (txsim ... TX System Internal Module)
 */
struct txsimbus_attach_args {
	char *tba_busname;
};

/* 
 *	txsim module attach arguments. 
 */
struct txsim_attach_args {
	tx_chipset_tag_t ta_tc; /* Chipset tag */
};

/*
 *	Interrupt staff
 */
#define MAKEINTR(s, b) ((s) * 32 + (ffs(b) - 1))
void*	tx_intr_establish __P((tx_chipset_tag_t, int, int, int, int (*) __P((void*)),	void*));
void	tx_intr_disestablish __P((tx_chipset_tag_t, void*));
#ifdef USE_POLL
void*	tx39_poll_establish __P((tx_chipset_tag_t, int, int, int, int (*) __P((void*)),	void*));
void	tx39_poll_disestablish __P((tx_chipset_tag_t, void*));
#endif /* USE_POLL */
void	tx_conf_register_intr __P((tx_chipset_tag_t, void*));

#ifdef TX39_DEBUG
extern u_int32_t tx39debugflag, tx39intrvec;
/*
 *	Debugging use.
 */
#define __bitdisp(a, s, e, m, c)					\
({									\
	u_int32_t __j, __j1;						\
	int __i, __s, __e, __n;						\
	__n = sizeof(typeof(a)) * NBBY - 1;				\
	__j1 = 1 << __n;						\
	__e = e ? e : __n;						\
	__s = s;							\
	for (__j = __j1, __i = __n; __j > 0; __j >>=1, __i--) {		\
		if (__i > __e || __i < __s) {				\
			printf("%c", a & __j ? '+' : '-');		\
		} else {						\
			printf("%c", a & __j ? '|' : '.');		\
		}							\
	}								\
	if (m) {							\
		printf("[%s]", (char*)m);				\
	}								\
	if (c) {							\
		for (__j = __j1, __i = __n; __j > 0; __j >>=1, __i--) {	\
			if (!(__i > __e || __i < __s) && (a & __j)) {	\
				printf(" %d", __i);			\
			}						\
		}							\
	}								\
	printf("\n");							\
})
#define bitdisp(a) __bitdisp(a, 0, 0, 0, 1)
#else /* TX39_DEBUG */
#define __bitdisp(a, s, e, m, c)
#define bitdisp(a)
#endif /* TX39_DEBUG */

int	__is_set_print __P((u_int32_t, int, char*));
