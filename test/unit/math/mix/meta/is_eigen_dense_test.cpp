#include <stan/math/mix.hpp>
#include <test/unit/math/mix/meta/eigen_utils.hpp>
#include <Eigen/Sparse>
#include <gtest/gtest.h>

TEST(MathMetaPrim, eigen_dense_tests) {
  using stan::math::var;
  using stan::math::fvar;
  using stan::is_eigen_dense;
  using stan::test::internal::eigen_mat;
  using stan::test::internal::eigen_arr;
  /**
   * Dense Flags:
   * eigen_base_v, eigen_dense_v, eigen_matrix_v, eigen_map_v, eigen_mat_exp_v,
   *  eigen_dense_solver_v
   *
   * Sparse Flags:
   * eigen_base_v, eigen_sparse_compressed_v, eigen_sparse_matrix_v,
   * eigen_sparse_map_v
   */
   test_all_eigen_dense_matrix<false, true, true, true, true, is_eigen_dense>();
   test_all_eigen_dense_array<false, true, true, true, true, is_eigen_dense>();
   test_all_eigen_dense_decomp<true, is_eigen_dense>();
   test_all_eigen_sparse<false, false, false, false, is_eigen_dense>();
}
