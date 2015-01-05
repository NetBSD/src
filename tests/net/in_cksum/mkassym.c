
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mbuf.h>
#include <stddef.h>
#include <stdio.h>

int
main(void)
{
	printf("#define M_LEN %zu\n", offsetof(struct m_hdr, mh_len));
	printf("#define M_DATA %zu\n", offsetof(struct m_hdr, mh_data));
	printf("#define M_NEXT %zu\n", offsetof(struct m_hdr, mh_next));
	return 0;
}
