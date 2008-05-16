/* $Id: imxuartvar.h,v 1.1.24.1 2008/05/16 02:21:56 yamt Exp $ */
/*
 * driver include for Freescale i.MX31 and i.MX31L UARTs
 */

typedef struct imxuart_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_bt;
	bus_space_handle_t	sc_bh;

	uint32_t		sc_intrspec_enb;
	uint32_t		sc_ucr[4];
	uint32_t		sc_usr[2];

	uint			sc_init_cnt;

	bus_addr_t		sc_addr;
	bus_size_t		sc_size;
	int			sc_intr;

	struct tty		*sc_tty;

	struct {
		ulong err;
		ulong brk;
		ulong prerr;
		ulong frmerr;
		ulong ovrrun;
	}			sc_errors;
} imxuart_softc_t;

int imxuart_init(imxuart_softc_t *, uint);
int imxuart_test(void);
