#define UHA_MSCP_MAX	32	/* store up to 32 MSCPs at one time */
#define	MSCP_HASH_SIZE	32	/* hash table size for phystokv */
#define	MSCP_HASH_SHIFT	9
#define MSCP_HASH(x)	((((long)(x))>>MSCP_HASH_SHIFT) & (MSCP_HASH_SIZE - 1))

struct uha_softc {
	struct device sc_dev;
	bus_chipset_tag_t sc_bc;

	bus_io_handle_t sc_ioh;
	int sc_irq, sc_drq;
	void *sc_ih;

	void (*start_mbox) __P((struct uha_softc *, struct uha_mscp *));
	int (*poll) __P((struct uha_softc *, struct scsi_xfer *, int));
	void (*init) __P((struct uha_softc *));

	struct uha_mscp *sc_mscphash[MSCP_HASH_SIZE];
	TAILQ_HEAD(, uha_mscp) sc_free_mscp;
	int sc_nummscps;
	int sc_scsi_dev;		/* our scsi id */
	struct scsi_link sc_link;
};

void	uha_attach __P((struct uha_softc *));
void	uha_timeout __P((void *arg));
struct	uha_mscp *uha_mscp_phys_kv __P((struct uha_softc *, u_long));
void	uha_done __P((struct uha_softc *, struct uha_mscp *));
