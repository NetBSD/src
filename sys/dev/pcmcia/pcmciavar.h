#include <sys/types.h>
#include <sys/queue.h>

#include <machine/bus.h>

#include <dev/pcmcia/pcmciachip.h>

/* pcmcia itself */

#define PCMCIA_CFE_MWAIT_REQUIRED	0x0001
#define PCMCIA_CFE_RDYBSY_ACTIVE	0x0002
#define PCMCIA_CFE_WP_ACTIVE		0x0004
#define PCMCIA_CFE_BVD_ACTIVE		0x0008
#define PCMCIA_CFE_IO8			0x0010
#define PCMCIA_CFE_IO16			0x0020
#define PCMCIA_CFE_IRQSHARE		0x0040
#define PCMCIA_CFE_IRQPULSE		0x0080
#define PCMCIA_CFE_IRQLEVEL		0x0100
#define PCMCIA_CFE_POWERDOWN		0x0200
#define PCMCIA_CFE_READONLY		0x0400
#define PCMCIA_CFE_AUDIO		0x0800

struct pcmcia_config_entry {
    int number;
    u_int32_t flags;
    int iftype;
    int num_iospace;
    u_long iomask;	/* the card will only decode this mask in any case, 
			   so we can do dynamic allocation with this in
			   mind, in case the suggestions below are no good */
    struct {
	u_long length;
	u_long start;
    } iospace[4];	/* XXX this could be as high as 16 */
    u_int16_t irqmask;
    int num_memspace;
    struct {
	u_long length;
	u_long cardaddr;
	u_long hostaddr;
    } memspace[2];	/* XXX this could be as high as 8 */
    int maxtwins;
    SIMPLEQ_ENTRY(pcmcia_config_entry) cfe_list;
};

struct pcmcia_function {
    /* read off the card */
    int number;
    int function;
    int last_config_index;
    u_long ccr_base;
    u_long ccr_mask;
    SIMPLEQ_HEAD(, pcmcia_config_entry) cfe_head;
    SIMPLEQ_ENTRY(pcmcia_function) pf_list;
    /* run-time state */
    struct pcmcia_softc *sc;
    struct pcmcia_config_entry *cfe;
    bus_space_tag_t ccrt;
    bus_space_handle_t ccrh;
    u_long ccr_offset;
    int ccr_window;
    pcmcia_mem_handle_t ccr_mhandle;
    u_long ccr_realsize;
    int (*ih_fct) __P((void *));
    void *ih_arg;
    int ih_ipl;
};

struct pcmcia_card {
    int cis1_major, cis1_minor;
    /* XXX waste of space? */
    char cis1_info_buf[256];
    char *cis1_info[4];
    int manufacturer;
    u_int16_t product;
    u_int16_t error;
    SIMPLEQ_HEAD(, pcmcia_function) pf_head;
};

struct pcmcia_softc {
    struct device dev;
    /* this stuff is for the socket */
    pcmcia_chipset_tag_t pct;
    pcmcia_chipset_handle_t pch;
    /* this stuff is for the card */
    struct pcmcia_card card;
    void *ih;
};

struct pcmcia_attach_args {
    u_int16_t manufacturer;
    u_int16_t product;
    struct pcmcia_card *card;
    struct pcmcia_function *pf;
};

struct pcmcia_tuple {
    unsigned int code;
    unsigned int length;
    u_long mult;
    u_long ptr;
    bus_space_tag_t memt;
    bus_space_handle_t memh;
};

void pcmcia_read_cis __P((struct pcmcia_softc *));
void pcmcia_print_cis __P((struct pcmcia_softc *));
int pcmcia_scan_cis __P((struct device *dev,
			 int (*)(struct pcmcia_tuple *, void *), void *));

#define pcmcia_cis_read_1(tuple, idx0) \
	(bus_space_read_1((tuple)->memt, (tuple)->memh, \
			  (tuple)->mult*(idx0)))
#define pcmcia_tuple_read_1(tuple, idx1) \
	(pcmcia_cis_read_1((tuple), ((tuple)->ptr+(2+(idx1)))))
#define pcmcia_tuple_read_2(tuple, idx2) \
	(pcmcia_tuple_read_1((tuple), (idx2)) | \
	 (pcmcia_tuple_read_1((tuple), (idx2)+1)<<8))
#define pcmcia_tuple_read_3(tuple, idx3) \
	(pcmcia_tuple_read_1((tuple), (idx3)) | \
	 (pcmcia_tuple_read_1((tuple), (idx3)+1)<<8) | \
	 (pcmcia_tuple_read_1((tuple), (idx3)+2)<<16))
#define pcmcia_tuple_read_4(tuple, idx4) \
	(pcmcia_tuple_read_1((tuple), (idx4)) | \
	 (pcmcia_tuple_read_1((tuple), (idx4)+1)<<8) | \
	 (pcmcia_tuple_read_1((tuple), (idx4)+2)<<16) | \
	 (pcmcia_tuple_read_1((tuple), (idx4)+3)<<24))
#define pcmcia_tuple_read_n(tuple, n, idxn) \
	(((n)==1)?pcmcia_tuple_read_1((tuple), (idxn)): \
	 (((n)==2)?pcmcia_tuple_read_2((tuple), (idxn)): \
	  (((n)==3)?pcmcia_tuple_read_3((tuple), (idxn)): \
	   /* n == 4 */ pcmcia_tuple_read_4((tuple), (idxn)))))

#define PCMCIA_SPACE_MEMORY	1
#define PCMCIA_SPACE_IO		2

int pcmcia_ccr_read __P((struct pcmcia_function *, int));
void pcmcia_ccr_write __P((struct pcmcia_function *, int, int));

#define pcmcia_mfc(sc) ((sc)->card.pf_head.sqh_first && \
			(sc)->card.pf_head.sqh_first->pf_list.sqe_next)

int pcmcia_enable_function __P((struct pcmcia_function *,
				struct device *,
				struct pcmcia_config_entry *));

#define pcmcia_io_alloc(pf, start, size, iotp, iohp) \
	(pcmcia_chip_io_alloc((pf)->sc->pct, pf->sc->pch, \
			      (start), (size), (iotp), (iohp)))

int pcmcia_io_map __P((struct pcmcia_function *, int, bus_size_t,
		       bus_space_tag_t, bus_space_handle_t, int *));

#define pcmcia_mem_alloc(pf, size, memtp, memhp, mhandle, realsize) \
	(pcmcia_chip_mem_alloc((pf)->sc->pct, (pf)->sc->pch, \
			       (size), (memtp), (memhp), (mhandle), \
			       (realsize)))
#define pcmcia_mem_free(pf, size, memt, memh, mhandle) \
	(pcmcia_chip_mem_free((pf)->sc->pct, (pf)->sc->pch, \
			      (size), (memt), (memh), (mhandle)))
#define pcmcia_mem_map(pf, kind, size, memt, memh, \
			    card_addr, offsetp, windowp) \
	(pcmcia_chip_mem_map((pf)->sc->pct, (pf)->sc->pch, \
			     (kind), (size), (memt), (memh), \
			     (card_addr), (offsetp), (windowp)))
#define pcmcia_mem_unmap(pf, window) \
	(pcmcia_chip_mem_unmap((pf)->sc->pct, (pf)->sc->pch, (window)))

void *pcmcia_intr_establish __P((struct pcmcia_function *, int, 
				 int (*)(void *), void *));
void pcmcia_intr_disestablish __P((struct pcmcia_function *, void *));

