
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <lzma.h>

static off_t
unxz(int i, int o, char *pre, size_t prelen, off_t *bytes_in)
{
	lzma_stream strm = LZMA_STREAM_INIT;
	lzma_ret ret;
	off_t x = 0;

	// Initialize the decoder
	ret = lzma_alone_decoder(&strm, UINT64_MAX);
	if (ret != LZMA_OK) {
		errno = ret == LZMA_MEM_ERROR ? ENOMEM : EINVAL;
		maybe_errx("Cannot initialize decoder");
	}

	// Input and output buffers
	uint8_t ibuf[BUFSIZ];
	uint8_t obuf[BUFSIZ];

	*bytes_in = prelen;
	strm.next_in = ibuf;
	strm.avail_in = read(i, ibuf + prelen, sizeof(ibuf) - prelen);
	if (strm.avail_in == (size_t)-1)
		maybe_errx("Read failed");

	memcpy(ibuf, pre, prelen);
	*bytes_in += strm.avail_in;

	strm.next_out = obuf;
	strm.avail_out = sizeof(obuf);
	if ((ret = lzma_stream_decoder(&strm, UINT64_MAX, 0)) != LZMA_OK)
		maybe_errx("Can't initialize decoder");

	for (;;) {
		if (strm.avail_in == 0) {
			strm.next_in = ibuf;
			strm.avail_in = read(i, ibuf, sizeof(ibuf));
//			fprintf(stderr, "read = %zu\n", strm.avail_in);
			if (strm.avail_in == (size_t)-1)
				maybe_errx("Read failed");
		}

		ret = lzma_code(&strm, LZMA_RUN);
//		fprintf(stderr, "ret = %d %zu %zu\n", ret, strm.avail_in, strm.avail_out);

		// Write and check write error before checking decoder error.
		// This way as much data as possible gets written to output
		// even if decoder detected an error.
		if (strm.avail_out == 0 || ret != LZMA_OK) {
			const size_t write_size = sizeof(obuf) - strm.avail_out;

			if (write(o, obuf, write_size) != (ssize_t)write_size)
				maybe_err("write failed");

			strm.next_out = obuf;
			strm.avail_out = sizeof(obuf);
			x += write_size;
		}

		if (ret != LZMA_OK) {
			if (ret == LZMA_STREAM_END) {
				// Check that there's no trailing garbage.
				if (strm.avail_in != 0 || read(i, ibuf, 1))
					ret = LZMA_DATA_ERROR;
				else {
					lzma_end(&strm);
					return x;
				}
			}

			const char *msg;
			switch (ret) {
			case LZMA_MEM_ERROR:
				msg = strerror(ENOMEM);
				break;

			case LZMA_FORMAT_ERROR:
				msg = "File format not recognized";
				break;

			case LZMA_OPTIONS_ERROR:
				// FIXME: Better message?
				msg = "Unsupported compression options";
				break;

			case LZMA_DATA_ERROR:
				msg = "File is corrupt";
				break;

			case LZMA_BUF_ERROR:
				msg = "Unexpected end of input";
				break;

			case LZMA_MEMLIMIT_ERROR:
				msg = "Reached memory limit";
				break;


			default:
				msg = "Internal error (bug)";
				break;
			}

			maybe_errx("%s", msg);
		}
	}
}
