#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

int main(int argc, char **argv)
{
	int fd, e, sno;
	char cmd[BUFSIZ], s[BUFSIZ];
	FILE *pp;

	if (argc < 5)
		errx(1, "usage: %s <fs-root> <raw-dev> <save-filename> "
		     "<work-filename>\n", argv[0]);

	fd = open(argv[1], 0, 0);
	if (fd < 0)
		err(1, argv[1]);

	fcntl(fd, LFCNWRAPGO, NULL);
	sleep(5);

	/* Loop forever calling LFCNWRAP{STOP,GO} */
	sno = 0;
	while(1) {
		printf("Waiting until fs wraps\n");
		fcntl(fd, LFCNWRAPSTOP, NULL);

		/*
		 * When the fcntl exits, the wrap is about to occur (but
		 * is waiting for the signal to go).  Call our mass-check
		 * script, and if all is well, continue.  The output
		 * of the script should end with a line that begins with a
		 * numeric code: zero for okay, nonzero for a failure.
		 */
		printf("Verifying all checkpoints from s/n %d\n", sno);
		sprintf(cmd, "./check-all %s %s %s %d", argv[2], argv[3],
			argv[4], sno);
		pp = popen(cmd, "r");
		s[0] = '\0';
		while(fgets(s, BUFSIZ, pp) != NULL)
			printf("  %s", s);
		if (s[0] == '\0') {
			printf("No checkpoints found or script exited\n");
			return 0;
		}
		sscanf(s, "%d %d", &e, &sno);
		if (e) {
			return 0;
		}
		pclose(pp);

		++sno;
		printf("Waiting until fs continues\n");
		fcntl(fd, LFCNWRAPGO, NULL);
	}

	return 0;
}
