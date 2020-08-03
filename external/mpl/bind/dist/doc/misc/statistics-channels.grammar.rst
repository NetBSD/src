::

  statistics-channels {
  	inet ( <ipv4_address> | <ipv6_address> |
  	    * ) [ port ( <integer> | * ) ] [
  	    allow { <address_match_element>; ...
  	    } ];
  };
