/*	$NetBSD: unbzip2.c,v 1.1.2.1 2004/03/31 09:05:36 grant Exp $	*/

/* This file is #included by gzip.c */

static off_t
unbzip2(int in, int out)
{
	FILE		*f_in;
	BZFILE		*b_in;
	int		bzerror, n;
	off_t		bytes_out = 0;
	char            buffer[64 * 1024];

	if ((in = dup(in)) < 0)
		maybe_err(1, "dup");

	if ((f_in = fdopen(in, "r")) == NULL)
		maybe_err(1, "fdopen");

	if ((b_in = BZ2_bzReadOpen(&bzerror, f_in, 0, 0, NULL, 0)) == NULL)
		maybe_err(1, "BZ2_bzReadOpen");

	do {
		n = BZ2_bzRead(&bzerror, b_in, buffer, sizeof (buffer));

		switch (bzerror) {
		case BZ_IO_ERROR:
			maybe_errx(1, "bzip2 I/O error");
		case BZ_UNEXPECTED_EOF:
			maybe_errx(1, "bzip2 unexpected end of file");
		case BZ_DATA_ERROR:
			maybe_errx(1, "bzip2 data integrity error");
		case BZ_DATA_ERROR_MAGIC:
			maybe_errx(1, "bzip2 magic number error");
		case BZ_MEM_ERROR:
			maybe_errx(1, "bzip2 out of memory");
		case BZ_OK:
		case BZ_STREAM_END:
			break;
		default:
			maybe_errx(1, "bzip2 unknown error");
		}

		if ((n = write(out, buffer, n)) < 0)
			maybe_err(1, "write");
		bytes_out += n;
	} while (bzerror != BZ_STREAM_END);

	(void)BZ2_bzReadClose(&bzerror, b_in);
	(void)fclose(f_in);


	return (bytes_out);
}
