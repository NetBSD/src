::

  dnssec-policy <string> {
  	dnskey-ttl <duration>;
  	keys { ( csk | ksk | zsk ) [ ( key-directory ) ] lifetime
  	    <duration_or_unlimited> algorithm <string> [ <integer> ]; ... };
  	max-zone-ttl <duration>;
  	parent-ds-ttl <duration>;
  	parent-propagation-delay <duration>;
  	parent-registration-delay <duration>;
  	publish-safety <duration>;
  	retire-safety <duration>;
  	signatures-refresh <duration>;
  	signatures-validity <duration>;
  	signatures-validity-dnskey <duration>;
  	zone-propagation-delay <duration>;
  };
