::

  zone <string> [ <class> ] {
  	type stub;
  	allow-query { <address_match_element>; ... };
  	allow-query-on { <address_match_element>; ... };
  	check-names ( fail | warn | ignore );
  	database <string>;
  	delegation-only <boolean>;
  	dialup ( notify | notify-passive | passive | refresh | <boolean> );
  	file <quoted_string>;
  	forward ( first | only );
  	forwarders [ port <integer> ] [ dscp <integer> ] { ( <ipv4_address> | <ipv6_address> ) [ port <integer> ] [ dscp <integer> ]; ... };
  	masterfile-format ( map | raw | text );
  	masterfile-style ( full | relative );
  	masters [ port <integer> ] [ dscp <integer> ] { ( <primaries> | <ipv4_address> [ port <integer> ] | <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	max-records <integer>;
  	max-refresh-time <integer>;
  	max-retry-time <integer>;
  	max-transfer-idle-in <integer>;
  	max-transfer-time-in <integer>;
  	min-refresh-time <integer>;
  	min-retry-time <integer>;
  	multi-master <boolean>;
  	primaries [ port <integer> ] [ dscp <integer> ] { ( <primaries> | <ipv4_address> [ port <integer> ] | <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	transfer-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	transfer-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	use-alt-transfer-source <boolean>;
  	zone-statistics ( full | terse | none | <boolean> );
  };
