::

  zone <string> [ <class> ] {
  	type redirect;
  	allow-query { <address_match_element>; ... };
  	allow-query-on { <address_match_element>; ... };
  	dlz <string>;
  	file <quoted_string>;
  	masterfile-format ( map | raw | text );
  	masterfile-style ( full | relative );
  	masters [ port <integer> ] [ dscp <integer> ] { ( <primaries> | <ipv4_address> [ port <integer> ] | <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	max-records <integer>;
  	max-zone-ttl ( unlimited | <duration> );
  	primaries [ port <integer> ] [ dscp <integer> ] { ( <primaries> | <ipv4_address> [ port <integer> ] | <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	zone-statistics ( full | terse | none | <boolean> );
  };
