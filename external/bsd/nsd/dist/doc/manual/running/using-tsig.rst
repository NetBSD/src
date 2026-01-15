Using Transaction Signature (TSIG)
==================================

NSD supports Transaction Signature (TSIG) for zone transfer and for notify
sending and receiving, for any query to the server.

TSIG keys are based on shared secrets. These must be configured in the config
file. To keep the secret in a separate file use ``include: "filename"`` to
include that file.

An example TSIG key named :file:`sec1_key`:

.. code:: text

    key:
      name: "sec1_key"
      algorithm: hmac-md5
      secret: "6KM6qiKfwfEpamEq72HQdA=="

This key can then be used for any query to the NSD server. NSD will check if the
signature is valid, and if so, return a signed answer. Unsigned queries will be
given unsigned replies.

The key can be used to restrict the access control lists, for example to only
allow zone transfer with the key, by listing the key name on the access control
line.

.. code:: text

    # provides AXFR to the subnet when TSIG is used.
    provide-xfr: 10.11.12.0/24 sec1_key
    # allow only notifications that are signed
    allow-notify: 192.168.0.0/16 sec1_key

If the TSIG key name is used in ``notify`` or ``request-xfr`` lines, the key is
used to sign the request/notification messages.