// Persistency_gtest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <gtest/gtest.h>

// Tests that private members can be accessed from a TEST declared as
// a friend of the class.
TEST(Persistency, Save) {
    int i = 1;
    EXPECT_EQ(1, i);
}
