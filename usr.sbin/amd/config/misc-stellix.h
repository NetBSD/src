#include <sys/fstyp.h>

struct ufs_args {
	char *fspec;
};

struct nfs_args {
        struct sockaddr_in      *addr;          /* file server address */
        fhandle_t               *fh;            /* File handle to be mounted */
        int                     flags;          /* flags */
        int                     wsize;          /* write size in bytes */
        int                     rsize;          /* read size in bytes */
        int                     timeo;          /* initial timeout in .1 secs *
/
        int                     retrans;        /* times to retry send */
        char                    *hostname;      /* server's name */
};
#define NFSMNT_SOFT     0x001   /* soft mount (hard is default) */
#define NFSMNT_WSIZE    0x002   /* set write size */
#define NFSMNT_RSIZE    0x004   /* set read size */
#define NFSMNT_TIMEO    0x008   /* set initial timeout (= 1.6 sec) */
#define NFSMNT_RETRANS  0x010   /* set number of request retrys */
#define NFSMNT_HOSTNAME 0x020   /* set hostname for error printf */
#define NFSMNT_INT	0x040	/* allow interrupts on hard mount */
