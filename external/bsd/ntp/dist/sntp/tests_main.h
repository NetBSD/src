/*	$NetBSD: tests_main.h,v 1.2.6.2 2014/12/25 02:34:42 snj Exp $	*/

#ifndef TESTS_MAIN_H
#define TESTS_MAIN_H

#include "config.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

extern "C" {
#include "ntp_stdlib.h"
}

class ntptest : public ::testing::Test {
public:
	static void SetExtraParams(int start, int count, char** argv);
protected:
	static std::vector<std::string> m_params;
};

#endif // TESTS_MAIN_H
