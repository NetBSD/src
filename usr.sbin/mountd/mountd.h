
#define	OP_MAPROOT	0x001
#define	OP_MAPALL	0x002
#define	OP_KERB		0x004
#define	OP_MASK		0x008
#define	OP_NET		0x010
#define	OP_ALLDIRS	0x040
#define OP_NORESPORT	0x080
#define OP_NORESMNT	0x100
#define OP_MASKLEN	0x200

extern int opt_flags;
extern int debug;
extern const int ninumeric;

struct netmsk {
	struct sockaddr_storage nt_net;
	int		nt_len;
	char           *nt_name;
};

int get_net(char *, struct netmsk *, int);
