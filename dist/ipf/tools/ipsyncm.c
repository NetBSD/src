/*	$NetBSD: ipsyncm.c,v 1.1.1.1 2004/03/28 08:56:35 martti Exp $	*/

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <net/if.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>

#include "netinet/ip_compat.h"
#include "netinet/ip_fil.h"
#include "netinet/ip_nat.h"
#include "netinet/ip_state.h"
#include "netinet/ip_sync.h"


int	main __P((int, char *[]));


int main(argc, argv)
int argc;
char *argv[];
{
	struct sockaddr_in sin;
	char buff[1400], *s;
	synclogent_t *sl;
	syncupdent_t *su;
	int nfd, lfd, n;
	synchdr_t *sh;

	if (argc < 2)
		exit(1);

	lfd = open(IPSYNC_NAME, O_RDONLY);
	if (lfd == -1) {
		perror("open");
		exit(1);
	}

	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(argv[1]);
	if (argc > 2)
		sin.sin_port = htons(atoi(argv[2]));
	else
		sin.sin_port = htons(43434);

	nfd = socket(AF_INET, SOCK_STREAM, 0);
	if (nfd == -1) {
		perror("socket");
		exit(1);
	}

	if (connect(nfd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		perror("connect");
		exit(1);
	}

	while ((n = read(lfd, buff, sizeof(buff))) > 0) {
		for (s = buff; s < buff + n; ) {
			sh = (synchdr_t *)s;
			printf("(%d) v:%d p:%d", (int)(buff + n - s), sh->sm_v,
			       sh->sm_p);

			if (sh->sm_cmd == SMC_CREATE)
				printf(" cmd:CREATE");
			else if (sh->sm_cmd == SMC_UPDATE)
				printf(" cmd:UPDATE");
			else
				printf(" cmd:Unknown(%d)", sh->sm_cmd);

			if (sh->sm_table == SMC_NAT)
				printf(" table:NAT");
			else if (sh->sm_table == SMC_STATE)
				printf(" table:STATE");
			else
				printf(" table:Unknown(%d)", sh->sm_table);

			printf(" num:%d\n", (u_32_t)ntohl(sh->sm_num));
			if (sh->sm_cmd == SMC_CREATE) {
				sl = (synclogent_t *)sh;
				s += sizeof(*sl);
			} else if (sh->sm_cmd == SMC_UPDATE) {
				su = (syncupdent_t *)sh;
				s += sizeof(*su);
			} else {
				printf("Unknown command\n");
			}
		}

		if (write(nfd, buff, n) != n) {
			perror("write");
			exit(1);
		}

	}
	close(lfd);
	close(nfd);
	exit(0);
}
