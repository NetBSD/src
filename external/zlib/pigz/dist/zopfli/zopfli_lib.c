/*
Copyright 2013 Google Inc. All Rights Reserved.
Author: lode@google.com (Lode Vandevenne)
*/

/*
Modified by madler@alumni.caltech.edu (Mark Adler)
Moved ZopfliInitOptions() to deflate.c.
*/

#include "zopfli.h"

#include "deflate.h"
#include "gzip_container.h"
#include "zlib_container.h"

#include <assert.h>

void ZopfliCompress(const ZopfliOptions* options, ZopfliFormat output_type,
                    const unsigned char* in, size_t insize,
                    unsigned char** out, size_t* outsize)
{
  if (output_type == ZOPFLI_FORMAT_GZIP) {
    ZopfliGzipCompress(options, in, insize, out, outsize);
  } else if (output_type == ZOPFLI_FORMAT_ZLIB) {
    ZopfliZlibCompress(options, in, insize, out, outsize);
  } else if (output_type == ZOPFLI_FORMAT_DEFLATE) {
    unsigned char bp = 0;
    ZopfliDeflate(options, 2 /* Dynamic block */, 1,
                  in, insize, &bp, out, outsize);
  } else {
    assert(0);
  }
}
