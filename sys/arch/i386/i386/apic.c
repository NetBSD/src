

#include <sys/types.h>
#include <sys/systm.h>

#include <machine/i82489reg.h>
#include <machine/i82489var.h>
#include <machine/apicvar.h>


const char redirlofmt[] = "\177\20"
	"f\0\10vector\0"
	"f\10\3delmode\0"
	"b\13logical\0"
	"b\14pending\0"
	"b\15actlo\0"
	"b\16irrpending\0"
	"b\17level\0"
	"b\20masked\0"
	"f\22\1dest\0" "=\1self" "=\2all" "=\3all-others";

const char redirhifmt[] = "\177\20"
	"f\30\10target\0";

void apic_format_redir (where1, where2, idx, redirhi, redirlo)
	char *where1;
	char *where2;
	int idx;
	u_int32_t redirhi;
	u_int32_t redirlo;
{
	char buf[256];

	printf("%s: %s%d %s",
	    where1, where2, idx,
	    bitmask_snprintf(redirlo, redirlofmt, buf, sizeof(buf)));

	if ((redirlo & LAPIC_DEST_MASK) == 0)
		printf(" %s",
		    bitmask_snprintf(redirhi, redirhifmt, buf, sizeof(buf)));

	printf("\n");
}

