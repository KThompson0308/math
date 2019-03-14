#ifndef STAN_MATH_PRIM_PROB_CAUCHY_RNG_HPP
#define STAN_MATH_PRIM_PROB_CAUCHY_RNG_HPP

#include <stan/math/prim/err/check_consistent_sizes.hpp>
#include <stan/math/prim/err/check_finite.hpp>
#include <stan/math/prim/err/check_positive_finite.hpp>
#include <stan/math/prim/meta/max_size.hpp>
#include <stan/math/prim/meta/scalar_seq_view.hpp>
#include <stan/math/prim/meta/VectorBuilder.hpp>
#include <boost/random/cauchy_distribution.hpp>
#include <boost/random/variate_generator.hpp>









namespace stan {
namespace math {

/**
 * Return a Cauchy random variate for the given location and scale
 * using the specified random number generator.
 *
 * mu and sigma can each be a scalar or a vector. Any non-scalar
 * inputs must be the same length.
 *
 * @tparam T_loc Type of location parameter
 * @tparam T_scale Type of scale parameter
 * @tparam RNG type of random number generator
 * @param mu (Sequence of) location parameter(s)
 * @param sigma (Sequence of) scale parameter(s)
 * @param rng random number generator
 * @return (Sequence of) Cauchy random variate(s)
 * @throw std::domain_error if mu is infinite or sigma is nonpositive
 * @throw std::invalid_argument if non-scalar arguments are of different
 * sizes
 */
template <typename T_loc, typename T_scale, class RNG>
inline typename VectorBuilder<true, double, T_loc, T_scale>::type cauchy_rng(
    const T_loc& mu, const T_scale& sigma, RNG& rng) {
  using boost::random::cauchy_distribution;
  using boost::variate_generator;
  static const char* function = "cauchy_rng";

  check_finite(function, "Location parameter", mu);
  check_positive_finite(function, "Scale parameter", sigma);
  check_consistent_sizes(function, "Location parameter", mu, "Scale Parameter",
                         sigma);

  scalar_seq_view<T_loc> mu_vec(mu);
  scalar_seq_view<T_scale> sigma_vec(sigma);
  size_t N = max_size(mu, sigma);
  VectorBuilder<true, double, T_loc, T_scale> output(N);

  for (size_t n = 0; n < N; ++n) {
    variate_generator<RNG&, cauchy_distribution<> > cauchy_rng(
        rng, cauchy_distribution<>(mu_vec[n], sigma_vec[n]));
    output[n] = cauchy_rng();
  }

  return output.data();
}

}  // namespace math
}  // namespace stan
#endif
