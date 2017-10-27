#include <gtest/gtest.h>
#include <boost/math/special_functions/fpclassify.hpp>
#include <limits>
#include <stan/math/prim/scal.hpp>

TEST(MathFunctions, sign) {
  double x;

  x = 0;
  EXPECT_EQ(0, stan::math::sign(x));
  x = 0.0000001;
  EXPECT_EQ(1, stan::math::sign(x));
  x = -0.001;
  EXPECT_EQ(-1, stan::math::sign(x));
}

TEST(MathFunctions, sign_nan) {
  double nan = std::numeric_limits<double>::quiet_NaN();

  EXPECT_EQ(1, stan::math::sign(nan));
}
