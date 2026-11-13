#!/usr/bin/env python3
# CHAP authentication: a server requiring CHAP must let a client with the
# right secret connect, and must refuse one with the wrong secret.

from pppfns import (
    EXIT_AUTH_TOPEER_FAILED, EXIT_HANGUP, EXIT_PEER_AUTH_FAILED,
    EXIT_PEER_DEAD, PppPair, require_link_env, test_fail,
)

require_link_env()

SECRETS_OK = 'cli\tsrv\t"s3cret"\t*\n'

# b is the server (requires CHAP) and looks the secret up in its
# chap-secrets file; a is the client and gets its secret from the
# "password" option, so only one side owns the secrets file (without
# namespaces both sides share the real confdir).
server_kwargs = dict(noauth=False, chap_secrets=SECRETS_OK)
server_options = ['auth', 'require-chap', 'name', 'srv']
client_options = ['user', 'cli', 'remotename', 'srv']

print('CHAP with correct secret:')
with PppPair(a_options=client_options + ['password', 's3cret'],
             b_options=server_options, b_kwargs=server_kwargs,
             name='good') as pair:
    pair.up()
    if 'Peer cli authenticated with CHAP' not in pair.b.log_text():
        test_fail('server log lacks "Peer cli authenticated with CHAP"')
    pair.down()

print('CHAP with wrong secret:')
with PppPair(a_options=client_options + ['password', 'wrong'],
             b_options=server_options, b_kwargs=server_kwargs,
             name='bad') as pair:
    pair.start()
    # The server must reject the client; both sides then exit on their own.
    # The client's exact exit code races between its own auth-failed path
    # and the hangup from the exiting server, so accept either family --
    # the strict assertions are the server's rejection and no link-up.
    pair.b.wait_for_text('Peer cli failed CHAP authentication')
    pair.a.expect_exit({EXIT_AUTH_TOPEER_FAILED, EXIT_PEER_AUTH_FAILED,
                        EXIT_PEER_DEAD, EXIT_HANGUP})
    if 'remote IP address' in pair.a.log_text():
        test_fail('link came up despite failed CHAP authentication')
