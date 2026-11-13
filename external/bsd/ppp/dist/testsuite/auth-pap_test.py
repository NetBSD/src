#!/usr/bin/env python3
# PAP authentication: a server requiring PAP must let a client with the
# right secret connect, and must refuse one with the wrong secret.

from pppfns import (
    EXIT_AUTH_TOPEER_FAILED, EXIT_HANGUP, EXIT_PEER_AUTH_FAILED,
    EXIT_PEER_DEAD, PppPair, require_link_env, test_fail,
)

require_link_env()

SECRETS_OK = 'cli\tsrv\t"s3cret"\t*\n'

# b is the server (requires PAP) and looks the secret up in its
# pap-secrets file; a is the client and gets its secret from the
# "password" option, so only one side owns the secrets file (without
# namespaces both sides share the real confdir).
server_kwargs = dict(noauth=False, pap_secrets=SECRETS_OK)
server_options = ['auth', 'require-pap', 'name', 'srv']
client_options = ['user', 'cli', 'remotename', 'srv']

print('PAP with correct secret:')
with PppPair(a_options=client_options + ['password', 's3cret'],
             b_options=server_options, b_kwargs=server_kwargs,
             name='good') as pair:
    pair.up()
    if 'Peer cli authenticated with PAP' not in pair.b.log_text():
        test_fail('server log lacks "Peer cli authenticated with PAP"')
    pair.down()

print('PAP with wrong secret:')
with PppPair(a_options=client_options + ['password', 'wrong'],
             b_options=server_options, b_kwargs=server_kwargs,
             name='bad') as pair:
    pair.start()
    # See auth-chap_test.py: the client's exit code races between its own
    # auth-failed path and the hangup from the exiting server.
    pair.b.wait_for_text('PAP peer authentication failed for cli')
    pair.a.expect_exit({EXIT_AUTH_TOPEER_FAILED, EXIT_PEER_AUTH_FAILED,
                        EXIT_PEER_DEAD, EXIT_HANGUP})
    if 'remote IP address' in pair.a.log_text():
        test_fail('link came up despite failed PAP authentication')
