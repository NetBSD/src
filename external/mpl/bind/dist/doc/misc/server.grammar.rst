::

  server <netprefix> {
  	bogus <boolean>;
  	edns <boolean>;
  	edns-udp-size <integer>;
  	edns-version <integer>;
  	keys <server_key>;
  	max-udp-size <integer>;
  	notify-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [
  	    dscp <integer> ];
  	notify-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * ) ]
  	    [ dscp <integer> ];
  	padding <integer>;
  	provide-ixfr <boolean>;
  	query-source ( ( [ address ] ( <ipv4_address> | * ) [ port (
  	    <integer> | * ) ] ) | ( [ [ address ] ( <ipv4_address> | * ) ]
  	    port ( <integer> | * ) ) ) [ dscp <integer> ];
  	query-source-v6 ( ( [ address ] ( <ipv6_address> | * ) [ port (
  	    <integer> | * ) ] ) | ( [ [ address ] ( <ipv6_address> | * ) ]
  	    port ( <integer> | * ) ) ) [ dscp <integer> ];
  	request-expire <boolean>;
  	request-ixfr <boolean>;
  	request-nsid <boolean>;
  	send-cookie <boolean>;
  	tcp-keepalive <boolean>;
  	tcp-only <boolean>;
  	transfer-format ( many-answers | one-answer );
  	transfer-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [
  	    dscp <integer> ];
  	transfer-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * )
  	    ] [ dscp <integer> ];
  	transfers <integer>;
  };
