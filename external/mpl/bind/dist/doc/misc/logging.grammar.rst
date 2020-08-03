::

  logging {
  	category <string> { <string>; ... };
  	channel <string> {
  		buffered <boolean>;
  		file <quoted_string> [ versions ( unlimited | <integer> ) ]
  		    [ size <size> ] [ suffix ( increment | timestamp ) ];
  		null;
  		print-category <boolean>;
  		print-severity <boolean>;
  		print-time ( iso8601 | iso8601-utc | local | <boolean> );
  		severity <log_severity>;
  		stderr;
  		syslog [ <syslog_facility> ];
  	};
  };
