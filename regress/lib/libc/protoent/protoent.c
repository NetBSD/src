#include <netdb.h>
#include <stdint.h>
#include <netinet/in.h>

int
main(int argc, char *argv[])
{
	struct protoent_data pvd;
	struct protoent *pvp, pv;
	char **pp;

	(void)memset(&pvd, 0, sizeof(pvd));
	setprotoent_r(0, &pvd);
	while ((pvp = getprotoent_r(&pv, &pvd)) != NULL) {
		printf("name=%s, port=%d, aliases=",
		    pvp->p_name, pvp->p_proto);
		for (pp = pvp->p_aliases; *pp; pp++)
			printf("%s ", *pp);
		printf("\n");
	}
	endprotoent_r(&pvd);
	return 0;
}
