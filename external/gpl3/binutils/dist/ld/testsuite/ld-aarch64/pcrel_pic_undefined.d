#name: PC-Rel relocation against undefined
#source: pcrel.s
#ld: -shared -e0
#warning: .*: relocation R_AARCH64_ADR_PREL_PG_HI21 against external symbol.*fPIC.*
#warning: .*: relocation R_AARCH64_ADR_PREL_PG_HI21_NC against external symbol.*fPIC.*
#warning: .*: relocation R_AARCH64_ADR_PREL_LO21 against external symbol.*fPIC.*
#warning: .*: relocation R_AARCH64_LD_PREL_LO19 against external symbol.*fPIC.*
#warning: .*: relocation R_AARCH64_PREL16 against external symbol.*fPIC.*
#warning: .*: relocation R_AARCH64_PREL32 against external symbol.*fPIC.*
#warning: .*: relocation R_AARCH64_PREL64 against external symbol.*fPIC.*
