#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>

#include "stand.h"
#include "net.h"
#include "netif.h"
#include "samachdep.h"

#ifdef DEBUG
int debug = 1;
#else
int debug = 0;
#endif

/* These get set by the bootp module */
extern char rootpath[], bootfile[], hostname[], domainname[];

/* Forwards */
extern int boot_nfs();
extern int boot_tftp();
extern int help();
extern int gettime();
extern int toggle_debug();
extern int cat();
#if 0
extern int name_res();
#endif
extern int set_server();
extern int set_gateway();
extern int set_address();
#ifdef LE_DEBUG
extern int toggle_le_debug();
#endif
#ifdef NETIF_DEBUG
extern int toggle_netif_debug();
#endif

struct cmds {
        char *cmd_name;
        int (*cmd_func)();
} cmd_table[] = {
{ "cat",	cat },
{ "nfs",	boot_nfs },
{ "tftp",	boot_tftp },
{ "time",	gettime },
#if 0
{ "resolv",	name_res },
#endif
#ifdef LE_DEBUG
{ "le_debug",	toggle_le_debug },
#endif
{ "debug",	toggle_debug },
#ifdef NETIF_DEBUG
{ "netif_debug",	toggle_netif_debug },
#endif
{ "server",	set_server },
{ "gateway",	set_gateway },
{ "address",	set_address },
{ "help",	help },
{ "?",		help },
{ NULL,		NULL }
};

gettime()
{
    printf("%d\n", getsecs());
}

set_server(s, argc, argv)
	int s, argc;
	char *argv[];
{
	if (argc != 2) {
		printf("usage: %s ip-addr\n", argv[0]);
		return;
	}
		
	rootip = ip_convertaddr(argv[1]);
	printf("server=%s\n", intoa(rootip));
}

set_address(s, argc, argv)
	int s, argc;
	char *argv[];
{
	if (argc != 2) {
		printf("usage: %s ip-addr\n", argv[0]);
		return;
	}
		
	myip = ip_convertaddr(argv[1]);
	printf("address=%s\n", intoa(myip));
}

set_gateway(s, argc, argv)
	int s, argc;
	char *argv[];
{
	if (argc != 2) {
		printf("usage: %s ip-addr\n", argv[0]);
		return;
	}
		
	gateip = ip_convertaddr(argv[1]);
	printf("address=%s\n", intoa(gateip));
}

toggle_debug(s, argc, argv)
	int s, argc;
	char *argv[];
{

	debug = !debug;
	printf("debug=%d\n", debug);
}

#ifdef LE_DEBUG
toggle_le_debug(s, argc, argv)
	int s, argc;
	char *argv[];
{
	extern int le_debug;
	
	le_debug = !le_debug;
	printf("le_debug=%d\n", le_debug);
}
#endif

#ifdef NETIF_DEBUG
toggle_netif_debug(s, argc, argv)
	int s, argc;
	char *argv[];
{
	extern int netif_debug;

	netif_debug = !netif_debug;
	printf("netif_debug=%d\n", netif_debug);
}
#endif

help()
{
        struct cmds *ct;

        printf("Commands are:\n");
        for (ct = cmd_table; ct->cmd_name; ct++)
                printf("%s\n", ct->cmd_name);
}

print_info()
{
	printf("myip: %s (%s)", hostname, intoa(myip));
	if (gateip)
		printf(", gateip: %s", intoa(gateip));
	if (mask)
		printf(", mask: %s", intoa(mask));
	printf("\n");
	printf("boot: %s:%s\n", intoa(rootip), bootfile);
	printf("root: %s:%s\n", intoa(rootip), rootpath);
	printf("swap: %s\n", intoa(swapip));
	printf("domain: %s, server: %s\n", domainname, intoa(nameip));
}

boot_nfs(sock, argc, argv)
	int sock, argc;
	char *argv[];
{
	struct stat st;
	int fd;
	char c;
	
	if (!myip) {
		bootp(sock);
		print_info();
	}

	/* mount root filesystem */
	printf("Mount %s:%s\n", intoa(rootip), rootpath);
	nfs_mount(sock, rootip, rootpath);
	printf("mounted");
	getchar();

	printf("Stat \"hello\"\n");
	if (stat("hello", &st) < 0)
		printf("stat: %s\n", strerror(errno));
	else {
		printf("%s: mode=%o uid=%d gid=%d size=%d\n", "hello",
			st.st_mode, st.st_uid, st.st_gid, st.st_size);
		getchar();
	}
	
	printf("Open hello\n");
	if ((fd = open("hello", 0)) < 0)
		printf("open: %s\n", strerror(errno));
	else
		printf("opened fd = %d\n", fd);
	getchar();

	printf("Reading \"hello\"\n");
	while (read(fd, &c, 1) == 1)
		putchar(c);
	printf("EOF");
	getchar();
	
	printf("Close hello\n");
	close(fd);
}

boot_tftp(sock, argc, argv)
	int sock, argc;
	char *argv[];
{
	if (!myip) {
		bootp(sock);
		print_info();
	}

}

#if 0
name_res(sock, argc, argv)
	int sock, argc;
	char *argv[];
{
	char buf[100], *np;

	if (argc < 2) {
		printf("resolv: ");
		gets(buf);
		if (*buf == '\004')
			return;
		np = buf;
	}
	else
		np = argv[1];
	
	if (!myip)
		bootp(sock);

	(void) resolve(sock, np);
}
#endif

cat(sock, argc, argv)
	int sock, argc;
	char *argv[];
{
	int fd;
	char c;
	struct stat st;
	char buf[100];

	printf("File: ");
	gets(buf);
	
	if (stat(buf, &st) < 0) {
		printf("stat: %s: %s\n", buf, strerror(errno));
		return;
	}
	
	printf("%s: mode=%o uid=%d gid=%d size=%d\n", buf,
		st.st_mode, st.st_uid, st.st_gid, st.st_size);
	getchar();
	
	if ((fd = open(buf, 0)) < 0) {
		printf("open: %s: %s\n", buf, strerror(errno));
		return;
	}
	printf("%s: open", buf);
	getchar();
	
	while (read(fd, &c, 1) == 1)
		putchar(c);

	printf("%s: EOF", buf);
	getchar();
	close(fd);
}

static int
getline(prompt, argv, argvsize)
	char **argv, *prompt;
	int argvsize;
{
	register char *p, ch;
	static char line[128];
	int argc = 0;

	if (prompt)
		printf("%s", prompt);
	
	gets(line);
	
	/* skip white spaces */
	for (p = line; isspace(*p); p++);

	while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0') {
		if (argc > argvsize) break;
		argv[(argc)++] = p;
		/* skip word */
		for (; *p != ' ' && *p != '\t' && *p != '\n' && *p != '\0'; p++);
		if (*p != '\0')
			*p++ ='\0';
		/* skip white spaces */
		for (; isspace(*p); p++);
	}
	argv[argc] = NULL;

	return(argc);
}

main()
{
	char *argv[8];
	int argc;
        struct cmds *ct;
	int s;
	
	bootdev = -1;	/* network */
	
	printf("\n>> NetBSD NETWORK BOOT HP9000/%s CPU [%s]\n",
	       getmachineid(), "$Revision: 1.3 $");

	s = netif_open(NULL);	/* XXX machdep may be "le" */
	if (s < 0)
		printf("Failed to open netif = %d: %s\n", s, strerror(errno));

	for (;;) {
                argc = getline("netboot> ", argv, NENTS(argv));
                for (ct = cmd_table; ct->cmd_name; ct++) {
                        if (strcmp(argv[0], ct->cmd_name) == 0) {
                                (*ct->cmd_func)(s, argc, argv);
				break;
                        }
                }
        }
	netif_close(s);
}
