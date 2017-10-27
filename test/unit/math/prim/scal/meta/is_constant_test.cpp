#include <gtest/gtest.h>
#include <boost/type_traits.hpp>
#include <stan/math/prim/scal.hpp>

TEST(MetaTraits, isConstant) {
  using stan::is_constant;

  EXPECT_TRUE(is_constant<double>::value);
  EXPECT_TRUE(is_constant<float>::value);
  EXPECT_TRUE(is_constant<unsigned int>::value);
  EXPECT_TRUE(is_constant<int>::value);
}
