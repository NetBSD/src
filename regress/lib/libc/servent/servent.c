#include <netdb.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>

int
main(int argc, char *argv[])
{
	struct servent_data svd;
	struct servent *svp, sv;
	char **pp;

	(void)memset(&svd, 0, sizeof(svd));
	setservent_r(0, &svd);
	while ((svp = getservent_r(&sv, &svd)) != NULL) {
		printf("name=%s, port=%d, proto=%s, aliases=",
		    svp->s_name, ntohs((uint16_t)svp->s_port), svp->s_proto);
		for (pp = svp->s_aliases; *pp; pp++)
			printf("%s ", *pp);
		printf("\n");
	}
	endservent_r(&svd);
	return 0;
}
