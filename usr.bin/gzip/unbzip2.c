/*	$NetBSD: unbzip2.c,v 1.1.2.2 2004/03/31 09:09:20 grant Exp $	*/

/* This file is #included by gzip.c */

#define INBUFSIZE	(64 * 1024)
#define OUTBUFSIZE	(64 * 1024)

static off_t
unbzip2(int in, int out)
{
	int		n, ret, end_of_file;
	off_t		bytes_out = 0;
	bz_stream	bzs;
	static char	*inbuf, *outbuf;

	if (inbuf == NULL && (inbuf = malloc(INBUFSIZE)) == NULL)
	        maybe_err(1, "malloc");
	if (outbuf == NULL && (outbuf = malloc(OUTBUFSIZE)) == NULL)
	        maybe_err(1, "malloc");

	bzs.bzalloc = NULL;
	bzs.bzfree = NULL;
	bzs.opaque = NULL;

	end_of_file = 0;
	ret = BZ2_bzDecompressInit(&bzs, 0, 0);
	if (ret != BZ_OK)
	        maybe_errx(1, "bzip2 init");

	bzs.avail_in = 0;

	while (ret != BZ_STREAM_END) {
	        if (bzs.avail_in == 0 && !end_of_file) {
	                n = read(in, inbuf, INBUFSIZE);
	                if (n < 0)
	                        maybe_err(1, "read");
	                if (n == 0)
	                        end_of_file = 1;
	                bzs.next_in = inbuf;
	                bzs.avail_in = n;
	        } else
	                n = 0;

	        bzs.next_out = outbuf;
	        bzs.avail_out = OUTBUFSIZE;
	        ret = BZ2_bzDecompress(&bzs);

	        switch (ret) {
	        case BZ_STREAM_END:
	        case BZ_OK:
	                if (ret == BZ_OK && end_of_file)
	                        maybe_err(1, "read");
	                if (!tflag) {
	                        n = write(out, outbuf, OUTBUFSIZE - bzs.avail_out);
	                        if (n < 0)
	                                maybe_err(1, "write");
	                }
	                bytes_out += n;
	                break;

	        case BZ_DATA_ERROR:
	                maybe_errx(1, "bzip2 data integrity error");
	        case BZ_DATA_ERROR_MAGIC:
	                maybe_errx(1, "bzip2 magic number error");
	        case BZ_MEM_ERROR:
	                maybe_errx(1, "bzip2 out of memory");
	        }
	}

	if (BZ2_bzDecompressEnd(&bzs) != BZ_OK)
	        return (0);

	return (bytes_out);
}
