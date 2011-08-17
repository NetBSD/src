
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <lzma.h>

static off_t
unxz(int i, int o, char *pre, size_t prelen, off_t *bytes_in)
{
	lzma_stream strm = LZMA_STREAM_INIT;
	static const int flags = LZMA_TELL_UNSUPPORTED_CHECK|LZMA_CONCATENATED;
	lzma_ret ret;
	lzma_action action = LZMA_RUN;
	off_t bytes_out, bp;
	uint8_t ibuf[BUFSIZ];
	uint8_t obuf[BUFSIZ];

	if (bytes_in == NULL)
		bytes_in = &bp;

	strm.next_in = ibuf;
	memcpy(ibuf, pre, prelen);
	strm.avail_in = read(i, ibuf + prelen, sizeof(ibuf) - prelen);
	if (strm.avail_in == (size_t)-1)
		maybe_err("read failed");
	strm.avail_in += prelen;
	*bytes_in = strm.avail_in;

	if ((ret = lzma_stream_decoder(&strm, UINT64_MAX, flags)) != LZMA_OK)
		maybe_errx("Can't initialize decoder (%d)", ret);

	strm.next_out = NULL;
	strm.avail_out = 0;
	if ((ret = lzma_code(&strm, LZMA_RUN)) != LZMA_OK)
		maybe_errx("Can't read headers (%d)", ret);

	bytes_out = 0;
	strm.next_out = obuf;
	strm.avail_out = sizeof(obuf);

	for (;;) {
		if (strm.avail_in == 0) {
			strm.next_in = ibuf;
			strm.avail_in = read(i, ibuf, sizeof(ibuf));
			switch (strm.avail_in) {
			case (size_t)-1:
				maybe_err("read failed");
				/*NOTREACHED*/
			case 0:
				action = LZMA_FINISH;
				break;
			default:
				*bytes_in += strm.avail_in;
				break;
			}
		}

		ret = lzma_code(&strm, action);

		// Write and check write error before checking decoder error.
		// This way as much data as possible gets written to output
		// even if decoder detected an error.
		if (strm.avail_out == 0 || ret != LZMA_OK) {
			const size_t write_size = sizeof(obuf) - strm.avail_out;

			if (write(o, obuf, write_size) != (ssize_t)write_size)
				maybe_err("write failed");

			strm.next_out = obuf;
			strm.avail_out = sizeof(obuf);
			bytes_out += write_size;
		}

		if (ret != LZMA_OK) {
			if (ret == LZMA_STREAM_END) {
				// Check that there's no trailing garbage.
				if (strm.avail_in != 0 || read(i, ibuf, 1))
					ret = LZMA_DATA_ERROR;
				else {
					lzma_end(&strm);
					return bytes_out;
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
				maybe_errx("Unknown error (%d)", ret);
				break;
			}
			maybe_errx("%s", msg);

		}
	}
}
