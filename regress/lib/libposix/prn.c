#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(void)
{
	int errors = 0;
	struct stat sb;

	(void)unlink("t1");
	(void)unlink("t2");
	if (creat("t1", 0600) < 0) {
		perror("create t1");
		exit(1);
	}

	if (link("t1", "t2")) {
		perror("link t1 t2");
		exit(1);
	}

	/* Check if rename to same name works as expected */
	if (rename("t1", "t1")) {
		perror("rename t1 t1");
		errors++;
	}
	if (stat("t1", &sb)) {
		perror("rename removed file? stat t1");
		exit(1);
	}

	if (rename("t1", "t2")) {
		perror("rename t1 t2");
		errors++;
	}
#if BSD_RENAME
	/* check if rename of hardlinked file works the BSD way */
	if (stat("t1", &sb)) {
		if (errno != ENOENT) {
			perror("BSD rename should remove file! stat t1");
			errors++;
		}
	} else {
		fprintf(stderr, "BSD rename should remove file!");
		errors++;
	}
#else
	/* check if rename of hardlinked file works as the standard says */
	if (stat("t1", &sb)) {
		perror("Posix rename should not remove file! stat t1");
		errors++;
	}
#endif

	/* check if we get the expected error */
	/* this also exercises icky shared libraries goo */
	if (rename("no/such/file/or/dir", "no/such/file/or/dir")) {
		if (errno != ENOENT) {
			perror("rename no/such/file/or/dir");
			errors++;
		}
	} else {
		fprintf(stderr, "No error renaming no/such/file/or/dir\n");
		errors++;
	}
	
	exit(errors ? 1 : 0);
}
