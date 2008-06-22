BEGIN {

    split("local lmtp relay smtp virtual", transports)

    vars["destination_concurrency_failed_cohort_limit"] = "default_destination_concurrency_failed_cohort_limit"
    vars["destination_concurrency_limit"] = "default_destination_concurrency_limit"
    vars["destination_concurrency_negative_feedback"] = "default_destination_concurrency_negative_feedback"
    vars["destination_concurrency_positive_feedback"] = "default_destination_concurrency_positive_feedback"
    vars["destination_recipient_limit"] = "default_destination_recipient_limit"
    vars["initial_destination_concurrency"] = "initial_destination_concurrency"
    vars["destination_rate_delay"] = "default_destination_rate_delay"

    # auto_table.h

    for (var in vars) {
	for (transport in transports) {
	    if (transports[transport] != "local" || (var != "destination_recipient_limit" && var != "destination_concurrency_limit"))
		print "\"" transports[transport] "_" var "\", \"$" vars[var] "\", &var_" transports[transport] "_" var ", 0, 0," > "auto_table.h" 
        }
	print "" > "auto_table.h"
    }

    # auto_vars.h

    for (var in vars) {
	for (transport in transports) {
	    if (transports[transport] != "local" || (var != "destination_recipient_limit" && var != "destination_concurrency_limit"))
		print "char *var_" transports[transport] "_" var ";" > "auto_vars.h"
	}
	print "" > "auto_vars.h"
    }
    exit(0)
}
