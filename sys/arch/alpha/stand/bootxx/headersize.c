#include <sys/types.h>
#include <sys/exec.h>
#include <machine/coff.h>

#define	HDR_BUFSIZE	8192

main()
{
	char buf[HDR_BUFSIZE];

	if (read(0, &buf, HDR_BUFSIZE) < HDR_BUFSIZE) {
		perror("read");
		exit(1);
	}

	printf("%d\n", N_COFFTXTOFF(*((struct filehdr *)buf),
	    *((struct aouthdr *)(buf + sizeof(struct filehdr)))) );
}
