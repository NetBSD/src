/* unbzip2.c -- decompress files in bzip2 format.
 */

#ifdef RCSID
static char rcsid[] = "$Id: unbzip2.c,v 1.1 1999/07/25 07:06:06 simonb Exp $";
#endif

#define BZ_NO_STDIO
#include <bzlib.h>
#include <stddef.h>

#include "gzip.h"


/* ============================================================================
 * Bunzip2 in to out.
 */
int unbzip2(in, out) 
    int in, out;    /* input and output file descriptors */
{
	int		n, ret, end_of_file;
	bz_stream	bzs;

	bzs.bzalloc = NULL;
	bzs.bzfree = NULL;
	bzs.opaque = NULL;

	end_of_file = 0;
	if (bzDecompressInit(&bzs, 0, 0) != BZ_OK)
		return(ERROR);

	/* Use up the remainder of "inbuf" that's been read in already */
	bzs.next_in = inbuf;
	bzs.avail_in = insize;

	while (1) {
		if (bzs.avail_in == 0 && !end_of_file) {
			n = read(in, inbuf, INBUFSIZ);
			if (n < 0)
				read_error();
			if (n == 0)
				end_of_file = 1;
			bzs.next_in = inbuf;
			bzs.avail_in = n;
		}

		bzs.next_out = outbuf;
		bzs.avail_out = OUTBUFSIZ;
		ret = bzDecompress(&bzs);

		if (ret == BZ_STREAM_END) {
			n = write(out, outbuf, OUTBUFSIZ - bzs.avail_out);
			if (n < 0)
				write_error();
			break;
		}
		else if (ret == BZ_OK) {
			if (end_of_file)
				read_error();
			n = write(out, outbuf, OUTBUFSIZ - bzs.avail_out);
		}
		else {
			switch (ret) {
			  case BZ_DATA_ERROR:
				error("bzip2 data integrity error");
			  case BZ_DATA_ERROR_MAGIC:
				error("bzip2 magic number error");
			  case BZ_MEM_ERROR:
				error("bzip2 out of memory");
			}
		}
	}

	if (bzDecompressEnd(&bzs) != BZ_OK)
		return(ERROR);

	return(OK);
}
