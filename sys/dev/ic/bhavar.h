/*
 * Mail box defs  etc.
 * these could be bigger but we need the bha_softc to fit on a single page..
 */
#define BHA_MBX_SIZE	32	/* mail box size  (MAX 255 MBxs) */
				/* don't need that many really */
#define BHA_CCB_MAX	32	/* store up to 32 CCBs at one time */
#define	CCB_HASH_SIZE	32	/* hash table size for phystokv */
#define	CCB_HASH_SHIFT	9
#define CCB_HASH(x)	((((long)(x))>>CCB_HASH_SHIFT) & (CCB_HASH_SIZE - 1))

#define bha_nextmbx(wmb, mbx, mbio) \
	if ((wmb) == &(mbx)->mbio[BHA_MBX_SIZE - 1])	\
		(wmb) = &(mbx)->mbio[0];		\
	else						\
		(wmb)++;

struct bha_mbx {
	struct bha_mbx_out mbo[BHA_MBX_SIZE];
	struct bha_mbx_in mbi[BHA_MBX_SIZE];
	struct bha_mbx_out *cmbo;	/* Collection Mail Box out */
	struct bha_mbx_out *tmbo;	/* Target Mail Box out */
	struct bha_mbx_in *tmbi;	/* Target Mail Box in */
};

struct bha_softc {
	struct device sc_dev;
	bus_chipset_tag_t sc_bc;

	bus_io_handle_t sc_ioh;
	int sc_irq, sc_drq;
	void *sc_ih;

	char sc_model[7],
	     sc_firmware[6];

	struct bha_mbx sc_mbx;		/* all our mailboxes */
#define	wmbx	(&sc->sc_mbx)
	struct bha_ccb *sc_ccbhash[CCB_HASH_SIZE];
	TAILQ_HEAD(, bha_ccb) sc_free_ccb, sc_waiting_ccb;
	int sc_numccbs, sc_mbofull;
	int sc_scsi_dev;		/* adapters scsi id */
	struct scsi_link sc_link;	/* prototype for devs */
};

int	bha_find __P((bus_chipset_tag_t, bus_io_handle_t, struct bha_softc *));
void	bha_attach __P((struct bha_softc *));
int	bha_intr __P((void *));
