#!/usr/bin/expect -f
########################################################################
# The Initial Developer of the Original Code is International
# Business Machines Corporation. Portions created by IBM
# Corporation are Copyright (C) 2007 International Business
# Machines Corporation. All Rights Reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the Common Public License as published by
# IBM Corporation; either version 1 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# Common Public License for more details.
#
# You should have received a copy of the Common Public License
# along with this program; if not, a copy can be viewed at
# http://www.opensource.org/licenses/cpl1.0.php.
#
#
# Program: tpm_changeownerauth01.sh
# Purpose: Test transitions between plain text, unicode and well-known
# for Owner and SRK passwords, using tpm_changeownerauth from tpm-tools
# package.
# Pre-Conditions: Owner and SRK password must be set to a plain text
# password before running this test. 
#
# Author: Ramon Brandão <ramongb@br.ibm.com>
# Date: Nov/07
# 
########################################################################

set timeout -1
set options {"-s" "-o"}

# States if the current owner password is the Well Known secret
set wnOpt ""
# States if the crrent owner password is a Unicode one
set isU ""

#Prompts for the owner and SRK Passwords
send_user "Enter the current Owner Password: "
expect_user -re "(.*)\n"
set ownerPasswd $expect_out(1,string)
send_user "Enter the current SRK Password: "
expect_user -re "(.*)\n"
set SRKPasswd $expect_out(1,string)

# Start of changeauth tests
foreach opt $options {

	if {$opt == "-s"} {
		send_user "\n\n#############################################################\n"
		send_user "####### Testing tpm_changeownerauth for SRK password:########\n"
		send_user "#############################################################\n\n"
	} else {
		send_user "\n\n###############################################################\n"
		send_user "####### Testing tpm_changeownerauth for OWNER password:########\n"
		send_user "###############################################################\n\n"
	}

	# Plaintext -> Unicode
	puts stdout "Test 1) tpm_changeownerauth \"$opt\": Changing password from PLAINTEXT to UNICODE:" 
	spawn tpm_changeownerauth $opt -n -l debug
	sleep 1
	expect "Enter owner password" {
		exp_send "$ownerPasswd\r"
		if {$opt == "-s"} {
			expect "Enter new SRK" {
				exp_send "$SRKPasswd\r"
				expect "Confirm" {
					exp_send "$SRKPasswd\r"
				}
			}
		} else {
			expect "Enter new owner" {
				exp_send "$ownerPasswd\r"
				expect "Confirm" {
					exp_send "$ownerPasswd\r"
					set isU "-g"
				}
			}
		}
		expect {
			"ngeAuth success" {
				expect "Tspi_Context_Close success" {
					send_user "PLAINTEXT to UNICODE auth change SUCCEED. \n\n"
				}
			}
			"*Auth*ailed" {
				expect "Tspi_Context_Close success" {
					send_user "PLAINTEXT to UNICODE auth change FAILED. \n\n"
				}
			}
		}
		
	}
	sleep 4

	# Unicode -> plaintext
	puts stdout "Test 2) tpm_changeownerauth \"$opt\": Changing password from UNICODE to PLAINTEXT:" 
	spawn tpm_changeownerauth $opt $isU -l debug
	sleep 1
	expect "Enter owner password" {
		exp_send "$ownerPasswd\r"
		if {$opt == "-s"} {
			expect "Enter new SRK" {
				exp_send "$SRKPasswd\r"
				expect "Confirm" {
					exp_send "$SRKPasswd\r"
				}
			}
		} else {
			expect "Enter new owner" {
				exp_send "$ownerPasswd\r"
				expect "Confirm" {
					exp_send "$ownerPasswd\r"
					set isU ""
				}
			}
		}
		expect {
			"ngeAuth success" {
				expect "Tspi_Context_Close success" {
					send_user "UNICODE to PLAINTEXT auth change SUCCEED. \n\n"
				}
			}
			"*Auth*ailed" {
				expect "Tspi_Context_Close success" {
					send_user "UNICODE to PLAINTEXT auth change FAILED. \n\n"
				}
			}
		}
	}
	sleep 4

	# Plaintext -> Well Known
	puts stdout "Test 3) tpm_changeownerauth \"$opt\": Changing password from PLAINTEXT to WELL KNOWN:" 
	spawn tpm_changeownerauth $opt -r -l debug
	sleep 1
	expect "Enter owner password" {
		exp_send "$ownerPasswd\r"
		if {$opt == "-o"} {
			set wnOpt "-z"
		}
		expect {
			"ngeAuth success" {
				expect "Tspi_Context_Close success" {
					send_user "PLAINTEXT to WELL KNOWN auth change SUCCEED. \n\n"
				}
			}
			"*Auth*ailed" {
				expect "Tspi_Context_Close success" {
					send_user "PLAINTEXT to WELL KNOWN auth change FAILED. \n\n"
				}
			}
		}
	}
	sleep 4
	
	# Well Known -> Unicode
	puts stdout "Test 4) tpm_changeownerauth \"$opt\": Changing password from WELL KNOWN to UNICODE:" 
	spawn tpm_changeownerauth $opt $wnOpt -n -l debug
	sleep 1
	if {$wnOpt != "-z"} {
		expect "Enter owner password" {exp_send "$ownerPasswd\r"}
	}
	if {$opt == "-s"} {
		expect "Enter new SRK" {
			exp_send "$SRKPasswd\r"
			expect "Confirm" {
				exp_send "$SRKPasswd\r"
			}
		}
	} else {
		expect "Enter new owner" {
			exp_send "$ownerPasswd\r"
			expect "Confirm" {
				exp_send "$ownerPasswd\r"
				set wnOpt ""
				set isU "-g"
			}
		}
	}
	expect {
		"ngeAuth success" {
			expect "Tspi_Context_Close success" {
				send_user "WELL KNOWN to UNICODE auth change SUCCEED. \n\n"
			}
		}	
		"*Auth*ailed" {
			expect "Tspi_Context_Close success" {
				send_user "WELL KNOWN to UNICODE auth change FAILED. \n\n"
			}
		}
	}	
	sleep 4

	# Unicode -> Well Known
	puts stdout "Test 5) tpm_changeownerauth \"$opt\": Changing password from UNICODE to WELL KNOWN:" 
	spawn tpm_changeownerauth $opt $isU -r -l debug
	sleep 1
	if {$wnOpt != "-z"} {
		expect "Enter owner password" {exp_send "$ownerPasswd\r"}
	}
	if {$opt == "-o"} {
		set wnOpt "-z"
	}
	expect {
		"ngeAuth success" {
			expect "Tspi_Context_Close success" {
				send_user "UNICODE to WELL KNOWN auth change SUCCEED. \n\n"
			}
		}
		"*Auth*ailed" {
			expect "Tspi_Context_Close success" {
				send_user "UNICODE to WELL KNOWN auth change FAILED. \n\n"
			}
		}
	}

	sleep 4
	# Well Known -> Plain - return to the starting state
	puts stdout "Test 6) tpm_changeownerauth \"$opt\": Changing password from WELL KNOWN to PLAIN:"
	spawn tpm_changeownerauth $opt $wnOpt -l debug
	sleep 1
	if {$wnOpt != "-z"} {
		expect "Enter owner password" {exp_send "$ownerPasswd\r"}
	}
	if {$opt == "-s" } {
		expect "Enter new SRK" {
			exp_send "$SRKPasswd\r"
			expect "Confirm" {
				exp_send "$SRKPasswd\r"
			}
		}	
	} else {
		expect "Enter new owner" {
			exp_send "$ownerPasswd\r"
			expect "Confirm" {
				exp_send "$ownerPasswd\r"
			}
		}
	}
	expect {
		"ngeAuth succes" {
			expect "Tspi_Context_Close success" {
				send_user "WELL KNOWN to PLAIN auth change SUCCEED. \n\n"
			}
		}
		"*Auth*ailed" {
			expect "Tspi_Context_Close success" {
				send_user "WELL KNOWN to PLAIN auth change FAILED. \n\n"
			}
		}
	}
	sleep 4
}

