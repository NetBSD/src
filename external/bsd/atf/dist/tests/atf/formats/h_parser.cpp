//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2009, 2010 The NetBSD Foundation, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

extern "C" {
#include <fcntl.h>
}

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include "atf-c/object.h"
}

#include "atf-c++/formats.hpp"
#include "atf-c++/io.hpp"
#include "atf-c++/parser.hpp"
#include "atf-c++/sanity.hpp"

class atffile_reader : protected atf::formats::atf_atffile_reader {
    void
    got_conf(const std::string& name, const std::string& val)
    {
        std::cout << "got_conf(" << name << ", " << val << ")" << std::endl;
    }

    void
    got_prop(const std::string& name, const std::string& val)
    {
        std::cout << "got_prop(" << name << ", " << val << ")" << std::endl;
    }

    void
    got_tp(const std::string& name, bool isglob)
    {
        std::cout << "got_tp(" << name << ", " << (isglob ? "true" : "false")
                  << ")" << std::endl;
    }

    void
    got_eof(void)
    {
        std::cout << "got_eof()" << std::endl;
    }

public:
    atffile_reader(std::istream& is) :
        atf::formats::atf_atffile_reader(is)
    {
    }

    void
    read(const std::string& outname, const std::string& errname)
    {
        atf_atffile_reader::read();
    }
};

class config_reader : protected atf::formats::atf_config_reader {
    void
    got_var(const std::string& name, const std::string& val)
    {
        std::cout << "got_var(" << name << ", " << val << ")" << std::endl;
    }

    void
    got_eof(void)
    {
        std::cout << "got_eof()" << std::endl;
    }

public:
    config_reader(std::istream& is) :
        atf::formats::atf_config_reader(is)
    {
    }

    void
    read(const std::string& outname, const std::string& errname)
    {
        atf_config_reader::read();
    }
};

class tcr_reader : protected atf::formats::atf_tcr_reader {
    void
    got_result(const std::string& str)
    {
        std::cout << "got_result(" << str << ")" << std::endl;
    }

    void
    got_reason(const std::string& str)
    {
        std::cout << "got_reason(" << str << ")" << std::endl;
    }

public:
    tcr_reader(std::istream& is) :
        atf::formats::atf_tcr_reader(is)
    {
    }

    void
    read(const std::string& outname, const std::string& errname)
    {
        atf_tcr_reader::read();
    }

    void
    got_eof(void)
    {
        std::cout << "got_eof()" << std::endl;
    }
};

class tp_reader : protected atf::formats::atf_tp_reader {
    void
    got_tc(const std::string& ident,
           const std::map< std::string, std::string >& md)
    {
        std::cout << "got_tc(" << ident << ", {";
        for (std::map< std::string, std::string >::const_iterator iter =
             md.begin(); iter != md.end(); iter++) {
            if (iter != md.begin())
                std::cout << ", ";
            std::cout << (*iter).first << '=' << (*iter).second;
        }
        std::cout << "})" << std::endl;
    }

    void
    got_eof(void)
    {
        std::cout << "got_eof()" << std::endl;
    }

public:
    tp_reader(std::istream& is) :
        atf::formats::atf_tp_reader(is)
    {
    }

    void
    read(const std::string& outname, const std::string& errname)
    {
        atf_tp_reader::read();
    }
};

class tps_reader : protected atf::formats::atf_tps_reader {
    void
    got_info(const std::string& what, const std::string& val)
    {
        std::cout << "got_info(" << what << ", " << val << ")" << std::endl;
    }

    void
    got_ntps(size_t ntps)
    {
        std::cout << "got_ntps(" << ntps << ")" << std::endl;
    }

    void
    got_tp_start(const std::string& tpname, size_t ntcs)
    {
        std::cout << "got_tp_start(" << tpname << ", " << ntcs << ")"
                  << std::endl;
    }

    void
    got_tp_end(const std::string& reason)
    {
        std::cout << "got_tp_end(" << reason << ")" << std::endl;
    }

    void
    got_tc_start(const std::string& tcname)
    {
        std::cout << "got_tc_start(" << tcname << ")" << std::endl;
    }

    void
    got_tc_end(const atf::tests::tcr& tcr)
    {
        std::string r;
        if (tcr.get_state() == atf::tests::tcr::passed_state)
            r = "passed";
        else if (tcr.get_state() == atf::tests::tcr::failed_state)
            r = "failed, " + tcr.get_reason();
        else if (tcr.get_state() == atf::tests::tcr::skipped_state)
            r = "skipped, " + tcr.get_reason();
        else
            UNREACHABLE;

        std::cout << "got_tc_end(" << r << ")" << std::endl;
    }

    void
    got_tc_stdout_line(const std::string& line)
    {
        std::cout << "got_tc_stdout_line(" << line << ")" << std::endl;
    }

    void
    got_tc_stderr_line(const std::string& line)
    {
        std::cout << "got_tc_stderr_line(" << line << ")" << std::endl;
    }

    void
    got_eof(void)
    {
        std::cout << "got_eof()" << std::endl;
    }

public:
    tps_reader(std::istream& is) :
        atf::formats::atf_tps_reader(is)
    {
    }

    void
    read(const std::string& outname, const std::string& errname)
    {
        atf_tps_reader::read();
    }
};

template< class R >
void
process(const std::string& file, const std::string& outname = "",
        const std::string& errname = "")
{
    try {
        std::ifstream is(file.c_str());
        if (!is)
            throw std::runtime_error("Cannot open `" + file + "'");
        R reader(is);
        reader.read(outname, errname);
    } catch (const atf::parser::parse_errors& pes) {
        std::cerr << pes.what() << std::endl;
    } catch (const atf::parser::parse_error& pe) {
        std::cerr << "LONELY PARSE ERROR: " << pe.first << ": "
                  << pe.second << std::endl;
    }
}

int
main(int argc, char* argv[])
{
    atf_init_objects();

    try {
        if (argc < 3) {
            std::cerr << "Missing arguments" << std::endl;
            return EXIT_FAILURE;
        }
        std::string format(argv[1]);
        std::string file(argv[2]);

        if (format == std::string("application/X-atf-atffile")) {
            process< atffile_reader >(file);
        } else if (format == std::string("application/X-atf-config")) {
            process< config_reader >(file);
        } else if (format == std::string("application/X-atf-tcr")) {
            process< tcr_reader >(file);
        } else if (format == std::string("application/X-atf-tp")) {
            process< tp_reader >(file);
        } else if (format == std::string("application/X-atf-tps")) {
            process< tps_reader >(file);
        } else {
            std::cerr << "Unknown format " << format << std::endl;
        }
    } catch (const atf::formats::format_error& e) {
        std::cerr << "Header format error: " << e.what() << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "UNEXPECTED ERROR: " << e.what() << std::endl;
    } catch (...) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
