#include <test/unit/math/test_ad.hpp>

TEST(mathMixCore, beta) {
  auto f = [](const auto& x1, const auto& x2) {
    using stan::math::lbeta;
    return lbeta(x1, x2);
  };

  stan::test::expect_common_nonzero_binary(f);

  stan::test::expect_ad(f, 0.5, 1.2);
  stan::test::expect_ad(f, 3.0, 6.0);
  stan::test::expect_ad(f, 3.1, 2.5);
  stan::test::expect_ad(f, 5.2, 6.7);
  stan::test::expect_ad(f, 6.7, 3.1);
  stan::test::expect_ad(f, 7.5, 1.8);
  stan::test::expect_ad(f, 12.3, 4.8);
}
