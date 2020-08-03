::

  zone <string> [ <class> ] {
  	type forward;
  	delegation-only <boolean>;
  	forward ( first | only );
  	forwarders [ port <integer> ] [ dscp <integer> ] { ( <ipv4_address> | <ipv6_address> ) [ port <integer> ] [ dscp <integer> ]; ... };
  };
