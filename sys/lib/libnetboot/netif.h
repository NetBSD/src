
struct netif {
    char *netif_bname;
    int netif_unit;
    int netif_exhausted;
    int (*netif_match) __P((void *, int *));
    int (*netif_probe) __P((void *));
    void (*netif_init) __P((struct iodesc *, void *));
    int (*netif_get) __P((struct iodesc *, void *, int, time_t));
    int (*netif_put) __P((struct iodesc *, void *, int));
    void (*netif_end) __P((void));
    struct netif_stats *netif_stats;
};

struct netif_stats {
    int collisions;
    int collision_error;
    int missed;
    int sent;
    int received;
    int deferred;
    int overflow;
};
extern struct netif *netiftab[];

extern int n_netif;

void netif_init __P((void));
struct netif *netif_select __P((void *));
int netif_probe __P((struct netif *, void *));
void netif_attach __P((struct netif *, struct iodesc *, void *));
void netif_detach __P((struct netif *));
