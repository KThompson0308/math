#ifndef STAN_MATH_PRIM_META_IS_AUTODIFF_HPP
#define STAN_MATH_PRIM_META_IS_AUTODIFF_HPP

#include <stan/math/prim/meta/scalar_type.hpp>
#include <stan/math/prim/meta/value_type.hpp>
#include <stan/math/prim/meta/require_helpers.hpp>
#include <complex>
#include <type_traits>

namespace stan {

  /**
   * Checks if decayed type is a var or fvar
   * @tparam The type to check
   */
  template <typename T>
  struct is_autodiff
      : bool_constant<math::disjunction<is_var<std::decay_t<T>>,
                                        is_fvar<std::decay_t<T>>>::value> {};

STAN_ADD_REQUIRE_UNARY(autodiff, is_autodiff);
STAN_ADD_REQUIRE_UNARY_SCALAR(autodiff, is_autodiff);
STAN_ADD_REQUIRE_UNARY_VALUE(autodiff, is_autodiff);

}  // namespace stan

#endif
