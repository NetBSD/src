/*	$NetBSD: unbzip2.c,v 1.1.2.3 2004/04/29 04:48:40 jmc Exp $	*/

/* This file is #included by gzip.c */

static off_t
unbzip2(int in, int out, char *pre, size_t prelen, off_t *bytes_in)
{
	int		ret, end_of_file;
	size_t		n;
	off_t		bytes_out = 0;
	bz_stream	bzs;
	static char	*inbuf, *outbuf;

	if (inbuf == NULL && (inbuf = malloc(BUFLEN)) == NULL)
	        maybe_err(1, "malloc");
	if (outbuf == NULL && (outbuf = malloc(BUFLEN)) == NULL)
	        maybe_err(1, "malloc");

	bzs.bzalloc = NULL;
	bzs.bzfree = NULL;
	bzs.opaque = NULL;

	end_of_file = 0;
	ret = BZ2_bzDecompressInit(&bzs, 0, 0);
	if (ret != BZ_OK)
	        maybe_errx(1, "bzip2 init");

	/* Prepend. */
	bzs.avail_in = prelen;
	bzs.next_in = pre;

	if (bytes_in)
		*bytes_in = prelen;

	while (ret != BZ_STREAM_END) {
	        if (bzs.avail_in == 0 && !end_of_file) {
	                n = read(in, inbuf, BUFLEN);
	                if (n < 0)
	                        maybe_err(1, "read");
	                if (n == 0)
	                        end_of_file = 1;
	                bzs.next_in = inbuf;
	                bzs.avail_in = n;
			if (bytes_in)
				*bytes_in += n;
	        }

	        bzs.next_out = outbuf;
	        bzs.avail_out = BUFLEN;
	        ret = BZ2_bzDecompress(&bzs);

	        switch (ret) {
	        case BZ_STREAM_END:
	        case BZ_OK:
	                if (ret == BZ_OK && end_of_file)
	                        maybe_err(1, "read");
	                if (!tflag) {
	                        n = write(out, outbuf, BUFLEN - bzs.avail_out);
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
