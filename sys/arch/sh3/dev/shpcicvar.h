/*	$NetBSD: shpcicvar.h,v 1.1.16.2 2002/03/16 15:59:35 jdolecek Exp $	*/

/*
 * Copyright (c) 1997 Marc Horowitz.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Marc Horowitz.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

struct proc;

struct shpcic_event {
	SIMPLEQ_ENTRY(shpcic_event) pe_q;
	int pe_type;
};

/* pe_type */
#define SHPCIC_EVENT_INSERTION	0
#define SHPCIC_EVENT_REMOVAL	1

struct shpcic_handle {
	struct shpcic_softc *sc;
	int	vendor;
	int	sock;
	int	flags;
	int	laststate;
	int	memalloc;
	struct {
		bus_addr_t	addr;
		bus_size_t	size;
		long		offset;
		int		kind;
	} mem[SHPCIC_MEM_WINS];
	int	ioalloc;
	struct {
		bus_addr_t	addr;
		bus_size_t	size;
		int		width;
	} io[SHPCIC_IO_WINS];
	int	ih_irq;
	struct device *pcmcia;

	int	shutdown;
	struct proc *event_thread;
	SIMPLEQ_HEAD(, shpcic_event) events;
};

/* These four lines are MMTA specific */
#define SHPCIC_IRQ1 10
#define SHPCIC_IRQ2 9
#define SHPCIC_SLOT1_ADDR 0xb8000000
#define SHPCIC_SLOT2_ADDR 0xb9000000

#define	SHPCIC_FLAG_SOCKETP	0x0001
#define	SHPCIC_FLAG_CARDP		0x0002

#define SHPCIC_LASTSTATE_PRESENT	0x0002
#define SHPCIC_LASTSTATE_HALF		0x0001
#define SHPCIC_LASTSTATE_EMPTY		0x0000

#define	C0SA SHPCIC_CHIP0_BASE+SHPCIC_SOCKETA_INDEX
#define	C0SB SHPCIC_CHIP0_BASE+SHPCIC_SOCKETB_INDEX
#define	C1SA SHPCIC_CHIP1_BASE+SHPCIC_SOCKETA_INDEX
#define	C1SB SHPCIC_CHIP1_BASE+SHPCIC_SOCKETB_INDEX

/*
 * This is sort of arbitrary.  It merely needs to be "enough". It can be
 * overridden in the conf file, anyway.
 */

#define	SHPCIC_MEM_PAGES	4
#define	SHPCIC_MEMSIZE	SHPCIC_MEM_PAGES*SHPCIC_MEM_PAGESIZE

#define	SHPCIC_NSLOTS	4

#define SHPCIC_WINS     5
#define SHPCIC_IOWINS     2

struct shpcic_softc {
	struct device dev;

	bus_space_tag_t memt;
	bus_space_handle_t memh;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	/* XXX isa_chipset_tag_t, pci_chipset_tag_t, etc. */
	void	*intr_est;

	pcmcia_chipset_tag_t pct;

	/* this needs to be large enough to hold PCIC_MEM_PAGES bits */
	int	subregionmask;
#define SHPCIC_MAX_MEM_PAGES (8 * sizeof(int))

	/* used by memory window mapping functions */
	bus_addr_t membase;

	/*
	 * used by io window mapping functions.  These can actually overlap
	 * with another pcic, since the underlying extent mapper will deal
	 * with individual allocations.  This is here to deal with the fact
	 * that different busses have different real widths (different pc
	 * hardware seems to use 10 or 12 bits for the I/O bus).
	 */
	bus_addr_t iobase;
	bus_addr_t iosize;

	int	irq;
	void	*ih;

	struct shpcic_handle handle[SHPCIC_NSLOTS];
};


int	shpcic_ident_ok(int);
int	shpcic_vendor(struct shpcic_handle *);
char	*shpcic_vendor_to_string(int);

void	shpcic_attach(struct shpcic_softc *);
void	shpcic_attach_sockets(struct shpcic_softc *);
int	shpcic_intr(void *arg);

static inline int shpcic_read(struct shpcic_handle *, int);
static inline void shpcic_write(struct shpcic_handle *, int, int);

int	shpcic_chip_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
	    struct pcmcia_mem_handle *);
void	shpcic_chip_mem_free(pcmcia_chipset_handle_t,
	    struct pcmcia_mem_handle *);
int	shpcic_chip_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t,
	    bus_size_t, struct pcmcia_mem_handle *, bus_size_t *, int *);
void	shpcic_chip_mem_unmap(pcmcia_chipset_handle_t, int);

int	shpcic_chip_io_alloc(pcmcia_chipset_handle_t, bus_addr_t,
	    bus_size_t, bus_size_t, struct pcmcia_io_handle *);
void	shpcic_chip_io_free(pcmcia_chipset_handle_t,
	    struct pcmcia_io_handle *);
int	shpcic_chip_io_map(pcmcia_chipset_handle_t, int, bus_addr_t,
	    bus_size_t, struct pcmcia_io_handle *, int *);
void	shpcic_chip_io_unmap(pcmcia_chipset_handle_t, int);

void	shpcic_chip_socket_enable(pcmcia_chipset_handle_t);
void	shpcic_chip_socket_disable(pcmcia_chipset_handle_t);

static __inline int shpcic_read(struct shpcic_handle *, int);
static __inline int
shpcic_read(struct shpcic_handle *h, int idx)
{
	static int prev_idx = 0;

	if (idx == -1){
		idx = prev_idx;
	}
	prev_idx = idx;
	return (bus_space_read_stream_2(h->sc->iot, h->sc->ioh, idx));
}

static __inline void shpcic_write(struct shpcic_handle *, int, int);
static __inline void
shpcic_write(struct shpcic_handle *h, int idx, int data)
{
	static int prev_idx;
	if (idx == -1){
		idx = prev_idx;
	}
	prev_idx = idx;
	bus_space_write_stream_2(h->sc->iot, h->sc->ioh, idx, (data));
}

void	*pcic_shb_chip_intr_establish(pcmcia_chipset_handle_t,
	    struct pcmcia_function *, int, int (*) (void *), void *);
void	pcic_shb_chip_intr_disestablish(pcmcia_chipset_handle_t, void *);
void	pcic_shb_bus_width_probe(struct shpcic_softc *, bus_space_tag_t,
	    bus_space_handle_t, bus_addr_t, u_int32_t);
