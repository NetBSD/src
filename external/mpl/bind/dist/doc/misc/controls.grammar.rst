::

  controls {
  	inet ( <ipv4_address> | <ipv6_address> |
  	    * ) [ port ( <integer> | * ) ] allow
  	    { <address_match_element>; ... } [
  	    keys { <string>; ... } ] [ read-only
  	    <boolean> ];
  	unix <quoted_string> perm <integer>
  	    owner <integer> group <integer> [
  	    keys { <string>; ... } ] [ read-only
  	    <boolean> ];
  };
