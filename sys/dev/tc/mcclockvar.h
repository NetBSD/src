
struct mcclock_softc {
	struct device sc_dev;
	const struct mcclock_busfns *sc_busfns;
};

struct mcclock_busfns {
	void    (*mc_bf_write) __P((struct mcclock_softc *, u_int, u_int));
	u_int   (*mc_bf_read) __P((struct mcclock_softc *, u_int));
};

void	mcclock_attach __P((struct mcclock_softc *,
	    const struct mcclock_busfns *));
