/*
 * Test program that reads, then writes a config file.
 */
#include <stdio.h>

#include "config.h"

int main(int argc, char **argv)
{
	struct config_file *cf;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
		exit(1);
	}

	cf = create_config_file();
	if (cf == NULL) {
		fprintf(stderr, "Couldn't create config_file object.\n");
		exit(1);
	}


	if (!read_config(cf, argv[1])) {
		fprintf(stderr, "Couldn't read config file '%s'\n", argv[0]);
		exit(1);
	}

	if (!write_config(cf, "out")) {
		fprintf(stderr, "Couldn't write config file 'out'\n");
		exit(1);
	}

	destroy_config_file(cf);
	dump_memory();
	return 0;
}
