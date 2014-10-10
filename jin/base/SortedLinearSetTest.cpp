#include "include/gtest/gtest.h"
#include "SortedLinearSet.h"

using namespace std;

TEST(SortedLinearSetTest, Basic)
{
    SortedLinearSet<string> s;

    s.insert("apple");
    s.insert("mango");
    s.insert("banana");

    EXPECT_EQ(1, s.count("apple"));
    EXPECT_EQ(0, s.count("melon"));

    s.insert("apple");
    s.insert("mango");
    s.insert("banana");
    EXPECT_EQ(3, s.size());
}
