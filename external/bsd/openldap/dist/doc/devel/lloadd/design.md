TODO:
- [ ] keep a global op in-flight counter? (might need locking)
- [-] scheduling (who does what, more than one select thread? How does the proxy
      work get distributed between threads?)
- [ ] managing timeouts?
- [X] outline locking policy: seems like there might be a lock inversion in the
      design looming: when working with op, might need a lock on both client and
      upstream but depending on where we started, we might want to start with
      locking one, then other
- [ ] how to deal with the balancer running out of fds? Especially when we hit
      the limit, then lose an upstream connection and accept() a client, we
      wouldn't be able to initiate a new one. A bit of a DoS... But probably not
      a concern for Ericsson
- [ ] non-Linux? No idea how anything other than poll works (moot if building a
      libevent/libuv-based load balancer since they take care of that, except
      edge-triggered I/O?)
- [-] rootDSE? Controls and exops might have different semantics and need
      binding to the same upstream connection.
- [ ] Just piggybacking on OpenLDAP as a module? Would still need some updates
      in the core and the module/subsystem would be a very invasive one. On the
      other hand, allows to expose live configuration and monitoring over LDAP
      over the current slapd listeners without re-inventing the wheel.


Expecting to handle only LDAPv3

terms:
  server - configured target
  upstream - a single connection to a server
  client - an incoming connection

To maintain fairness `G( requested => ( F( progressed | failed ) ) )`, use
queues and put timeouts in

Runtime organisation
------
- main thread with its own event base handling signals
- one thread (later possibly more) listening on the rendezvous sockets, handing
  the new sockets to worker threads
- n worker threads dealing with client and server I/O (dispatching actual work
  to the thread pool most likely)
- a thread pool to handle actual work

Operational behaviour
------

- client read -> upstream write:
  - client read:
    - if TLS_SETUP, keep processing, set state back when finished and note that
      we're under TLS
    - ber_get_next(), if we don't have a tag, finished (unless we have true
      edge-triggered I/O, also put the fd back into the ones we're waiting for)
    - peek at op tag:
      - unbind:
        - with a single lock, mark all pending ops in upstreams abandoned, clear
          client link (would it be fast enough if we remove them from upstream
          map instead?)
        - locked per op:
          - remove op from upstream map
          - check upstream is not write-suspended, if it is ...
          - try to write the abandon op to upstream, suspend upstream if not
            fully sent
          - remove op from client map (how if we're in avl_apply?, another pass?)
        - would be nice if we could wipe the complete client map then, otherwise
          we need to queue it to have it freed when all abandons get passed onto
          the upstream (just dropping them might put extra strain on upstreams,
          will probably have a queue on each client/upstream anyway, not just a
          single Ber)
      - bind:
        - check mechanism is not EXTERNAL (or implement it)
        - abandon existing ops (see unbind)
        - set state to BINDING, put DN into authzid
        - pick upstream, create PDU and sent
      - abandon:
        - find op, mark for abandon, send to appropriate upstream
      - Exop:
        - check not BINDING (unless it's a cancel?)
        - check OID:
          - STARTTLS:
            - check we don't have TLS yet
            - abandon all
            - set state to TLS_SETUP
            - send the hello
          - VC(?):
            - similar to bind except for the abandons/state change
      - other:
        - check not BINDING
        - pick an upstream
        - create a PDU, send (marking upstream suspended if not written in full)
      - check if should read again (keep a counter of number of times to read
        off a connection in a single pass so that we maintain fairness)
      - if read enough requests and can still read, re-queue ourselves (if we
        don't have true edge-triggered I/O, we can just register the fd again)
  - upstream write (only when suspended):
    - flush the current BER
    - there shouldn't be anything else?
- upstream read -> client write:
  - upstream read:
    - ber_get_next(), if we don't have a tag, finished (unless we have true
      edge-triggered I/O, also put the fd back into the ones we're waiting for)
    - when we get it, peek at msgid, resolve client connection, lock, check:
      - if unsolicited, handle as close (and mark connection closing)
      - if op is abandoned or does not exist, drop PDU and op, update counters
      - if client backlogged, suspend upstream, register callback to unsuspend
        (on progress when writing to client or abandon from client (connection
        death, abandon proper, ...))
    - reconstruct final PDU, write BER to client, if did not write fully,
      suspend client
    - if a final response, decrement operation counts on upstream and client
    - check if should read again (keep a counter of number of responses to read
      off a connection in a single pass so that we don't starve any?)
  - client write ready (only checked for when suspended):
    - write the rest of pending BER if any
    - on successful write, pick all pending ops that need failure response, push
      to client (are there any controls that need to be present in response even
      in the case of failure?, what to do with them?)
    - on successfully flushing them, walk through suspended upstreams, picking
      the pending PDU (unsuspending the upstream) and writing, if PDU flushed
      successfully, pick next upstream
    - if we successfully flushed all suspended upstreams, unsuspend client
      (and disable the write callback)
- upstream close/error:
  - look up pending ops, try to write to clients, mark clients suspended that
    have ops that need responses (another queue associated with client to speed
    up?)
  - schedule a new connection open
- client close/error:
  - same as unbind
- client inactive (no pending ops and nothing happened in x seconds)
  - might just send notice of disconnection and close
- op timeout handling:
  - mark for abandon
  - send abandon
  - send timeLimitExceeded/adminLimitExceeded to client

Picking an upstream:
- while there is a level available:
  - pick a random ordering of upstreams based on weights
  - while there is an upstream in the level:
    - check number of ops in-flight (this is where we lock the upstream map)
    - find the least busy connection (and check if a new connection should be
      opened)
    - try to lock for socket write, if available (no BER queued) we have our
      upstream

PDU processing:
- request (have an upstream selected):
  - get new msgid from upstream
  - create an Op structure (actually, with the need for freelist lock, we can
    make it a cache for freed operation structures, avoiding some malloc
    traffic, to reset, we need slap_sl_mem_create( ,,, 1 ))
  - check proxyauthz is not present? or just let upstream reject it if there are
    two?
  - add own controls at the end:
    - construct proxyauthz from authzid
    - construct session tracking from remote IP, own name, authzid
  - send over
  - insert Op into client and upstream maps
- response/intermediate/entry:
  - look up Op in upstream's map
  - write old msgid, rest of the response can go unchanged
  - if a response, remove Op from all maps (client and upstream)

Managing upstreams:
- async connect up to min_connections (is there a point in having a connection
  count range if we can't use it when needed since all of the below is async?)
- when connected, set up TLS (if requested)
- when done, send a bind
- go for the bind interaction
- when done, add it to the upstream's connection list
- (if a connection is suspended or connections are over 75 % op limit, schedule
  creating a new connection setup unless connection limit has been hit)

Managing timeouts:
- two options:
  - maintain a separate locked priority queue to give a perfect ordering to when
    each operation is to time out, would need to maintain yet another place
    where operations can be found.
    - the locking protocol for disposing of the operation would need to be
      adjusted and might become even more complicated, might do the alternative
      initially and then attempt this if it helps performance
  - just do a sweep over all clients (that mutex is less contended) every so
    often. With many in-flight operations might be a lot of wasted work.
    - we still need to sweep over all clients to check if they should be killed
      anyway

Dispatcher thread (2^n of them, fd x is handled by thread no x % (2^n)):
- poll on all registered fds
- remove each fd that's ready from the registered list and schedule the work
- work threads can put their fd back in if they deem necessary (=not suspended)
- this works as a poor man's edge-triggered polling, with enough workers, should
  we do proper edge triggered I/O? What about non-Linux?

Listener thread:
- slapd has just one, which then reassigns the sockets to separate I/O
  threads

Threading:
- if using slap_sl_malloc, how much perf do we gain? To allocate a context per
  op, we should have a dedicated parent context so that when we free it, we can
  use that exclusively. The parent context's parent would be the main thread's
  context. This implies a lot of slap_sl_mem_setctx/slap_sl_mem_create( ,,, 0 )
  and making sure an op does not allocate/free things from two threads at the
  same time (might need an Op mutex after all? Not such a huge cost if we
  routinely reuse Op structures)

Locking policy:
- read mutexes are unnecessary, we only have one thread receiving data from the
  connection - the one started from the dispatcher
- two reference counters of operation structures (an op is accessible from
  client and upstream map, each counter is consistent when thread has a lock on
  corresponding map), when decreasing the counter to zero, start freeing
  procedure
- place to mark disposal finished for each side, consistency enforced by holding
  the freelist lock when reading/manipulating
- when op is created, we already have a write lock on upstream socket and map,
  start writing, insert to upstream map with upstream refcount 1, unlock, lock
  client, insert (client refcount 0), unlock, lock upstream, decrement refcount
  (triggers a test if we need to drop it now), unlock upstream, done
- when upstream processes a PDU, locks its map, increments counter, (potentially
  removes if it's a response), unlocks, locks client's map, write mutex (this
  order?) and full client mutex (if a bind response)
- when client side wants to work with a PDU (abandon, (un)bind), locks its map,
  increase refcount, unlocks, locks upstream map, write mutex, sends or queues
  abandon, unlocks write mutex, initiates freeing procedure from upstream side
  (or if having to remember we've already increased client-side refcount, mark
  for deletion, lose upstream lock, lock client, decref, either triggering
  deletion from client or mark for it)
- if we have operation lock, we can simplify a bit (no need for three-stage
  locking above)

Shutdown:
- stop accept() thread(s) - potentially add a channel to hand these listening
  sockets over for zero-downtime restart
- if very gentle, mark connections as closing, start timeout and:
  - when a new non-abandon PDU comes in from client - return LDAP_UNAVAILABLE
  - when receiving a PDU from upstream, send over to client, if no ops pending,
    send unsolicited response and close (RFC4511 suggests unsolicited response
    is the last PDU coming from the upstream and libldap agrees, so we can't
    send it for a socket we want to shut down more gracefully)
- gentle (or very gentle timed out):
  - set timeout
  - mark all ops as abandoned
  - send unbind to all upstreams
  - send unsolicited to all clients
- imminent (or gentle timed out):
  - async close all connections?
  - exit()

RootDSE:
- default option is not to care and if a control/exop has special restrictions,
  it is the admin's job to flag it as such in the load-balancer's config
- another is not to care about the search request but check each search entry
  being passed back, check DN and if it's a rootDSE, filter the list of
  controls/exops/sasl mechs (external!) that are supported
- last one is to check all search requests for the DN/scope and synthesise the
  response locally - probably not (would need to configure the complete list of
  controls, exops, sasl mechs, naming contexts in the balancer)

Potential red flags:
- we suspend upstreams, if we ever suspend clients we need to be sure we can't
  create dependency cycles
  - is this an issue when only suspending the read side of each? Because even if
    we stop reading from everything, we should eventually flush data to those we
    can still talk to, as upstreams are flushed, we can start sending new
    requests from live clients (those that are suspended are due to their own
    inability to accept data)
  - we might need to suspend a client if there is a reason to choose a
    particular upstream (multi-request operation - bind, VC, PR, TXN, ...)
    - a SASL bind, but that means there are no outstanding ops to receive
      it holds that !suspended(client) \or !suspended(upstream), so they
      cannot participate in a cycle
    - VC - multiple binds at the same time - !!! more analysis needed
    - PR - should only be able to have one per connection (that's a problem
      for later, maybe even needs a dedicated upstream connection)
    - TXN - ??? probably same situation as PR
  - or if we have a queue for pending Bers on the server, we not need to suspend
    clients, upstream is only chosen if the queue is free or there is a reason
    to send it to that particular upstream (multi-stage bind/VC, PR, ...), but
    that still makes it possible for a client to exhaust all our memory by
    sending requests (VC or other ones bound to a slow upstream or by not
    reading the responses at all)
