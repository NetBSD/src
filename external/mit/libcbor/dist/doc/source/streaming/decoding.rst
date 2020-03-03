Decoding
=============================

Another way to decode data using libcbor is to specify a callbacks that will be invoked when upon finding certain items in the input. This service is provided by

.. doxygenfunction:: cbor_stream_decode


To get started, you might want to have a look at the simple streaming example:

.. code-block:: c

    #include "cbor.h"
    #include <stdio.h>
    #include <string.h>

    /*
     * Illustrates how one might skim through a map (which is assumed to have
     * string keys and values only), looking for the value of a specific key
     *
     * Use the examples/data/map.cbor input to test this.
     */

    const char * key = "a secret key";
    bool key_found = false;

    void find_string(void * _ctx, cbor_data buffer, size_t len)
    {
        if (key_found) {
            printf("Found the value: %*s\n", (int) len, buffer);
            key_found = false;
        } else if (len == strlen(key)) {
            key_found = (memcmp(key, buffer, len) == 0);
        }
    }

    int main(int argc, char * argv[])
    {
        FILE * f = fopen(argv[1], "rb");
        fseek(f, 0, SEEK_END);
        size_t length = (size_t)ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char * buffer = malloc(length);
        fread(buffer, length, 1, f);

        struct cbor_callbacks callbacks = cbor_empty_callbacks;
        struct cbor_decoder_result decode_result;
        size_t bytes_read = 0;
        callbacks.string = find_string;
        while (bytes_read < length) {
            decode_result = cbor_stream_decode(buffer + bytes_read,
                                               length - bytes_read,
                                               &callbacks, NULL);
            bytes_read += decode_result.read;
        }

        free(buffer);
        fclose(f);
    }

The callbacks are defined by

.. doxygenstruct:: cbor_callbacks
    :members:

When building custom sets of callbacks, feel free to start from

.. doxygenvariable:: cbor_empty_callbacks

Related structures
~~~~~~~~~~~~~~~~~~~~~

.. doxygenenum:: cbor_decoder_status
.. doxygenstruct:: cbor_decoder_result
    :members:


Callback types definition
~~~~~~~~~~~~~~~~~~~~~~~~~~~~


.. doxygentypedef:: cbor_int8_callback
.. doxygentypedef:: cbor_int16_callback
.. doxygentypedef:: cbor_int32_callback
.. doxygentypedef:: cbor_int64_callback
.. doxygentypedef:: cbor_simple_callback
.. doxygentypedef:: cbor_string_callback
.. doxygentypedef:: cbor_collection_callback
.. doxygentypedef:: cbor_float_callback
.. doxygentypedef:: cbor_double_callback
.. doxygentypedef:: cbor_bool_callback
