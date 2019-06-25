#include <stan/math/prim/mat.hpp>
#include <gtest/gtest.h>
#include <vector>

typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> const_t1;
typedef std::vector<const_t1> const_t2;
typedef std::vector<const_t2> const_t3;

typedef Eigen::Matrix<double, Eigen::Dynamic, 1> const_u1;
typedef std::vector<const_u1> const_u2;
typedef std::vector<const_u2> const_u3;

typedef Eigen::Matrix<double, 1, Eigen::Dynamic> const_v1;
typedef std::vector<const_v1> const_v2;
typedef std::vector<const_v2> const_v3;

TEST(MetaTraits, isConstantStruct) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::is_constant_struct;

  EXPECT_TRUE(is_constant_all<const_t1>::value);
  EXPECT_TRUE(is_constant_all<const_t2>::value);
  EXPECT_TRUE(is_constant_all<const_t3>::value);
  EXPECT_TRUE(is_constant_all<const_u1>::value);
  EXPECT_TRUE(is_constant_all<const_u2>::value);
  EXPECT_TRUE(is_constant_all<const_u3>::value);
  EXPECT_TRUE(is_constant_all<const_v1>::value);
  EXPECT_TRUE(is_constant_all<const_v2>::value);
  EXPECT_TRUE(is_constant_all<const_v3>::value);
  bool temp = is_constant_all<const_t1, const_t2>::value;
  EXPECT_TRUE(temp);
  temp = is_constant_all<const_t2, const_t3, const_u1>::value;
  EXPECT_TRUE(temp);
  temp = is_constant_all<const_u1, const_v1, const_v2, const_t2>::value;
  EXPECT_TRUE(temp);
}
