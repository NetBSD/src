/*	$NetBSD: tests_main.h,v 1.1.1.1.4.2 2014/05/22 15:50:12 yamt Exp $	*/

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
