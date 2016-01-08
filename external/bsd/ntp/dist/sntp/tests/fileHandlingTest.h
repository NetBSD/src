#ifndef FILE_HANDLING_TEST_H
#define FILE_HANDLING_TEST_H

#include "config.h"
#include "stdlib.h"
#include "sntptest.h"

#include <string.h>
#include <unistd.h>


enum DirectoryType {
	INPUT_DIR = 0,
	OUTPUT_DIR = 1
};

#define SRCDIR_DEF "/usr/src/external/bsd/ntp/ntp-4.2.8p5/sntp/tests/data/";
//const char srcdir[] = "/usr/src/external/bsd/ntp/ntp-4.2.8p5/sntp/tests/data/";

const char *
CreatePath(const char* filename, enum DirectoryType argument);


int
GetFileSize(FILE *file);


bool
CompareFileContent(FILE* expected, FILE* actual);

void
ClearFile(const char * filename) ;


#endif // FILE_HANDLING_TEST_H
