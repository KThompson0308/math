#ifndef STAN_MATH_REV_FUNCTOR_INTEGRATE_ODE_CVODES_HPP
#define STAN_MATH_REV_FUNCTOR_INTEGRATE_ODE_CVODES_HPP

#include <stan/math/rev/meta.hpp>
#include <stan/math/rev/functor/cvodes_utils.hpp>
#include <stan/math/rev/functor/coupled_ode_system.hpp>
#include <stan/math/rev/fun/ode_store_sensitivities.hpp>
#include <stan/math/prim/err.hpp>
#include <stan/math/prim/fun/value_of.hpp>
#include <cvodes/cvodes.h>
#include <sunlinsol/sunlinsol_dense.h>
#include <algorithm>
#include <ostream>
#include <vector>

namespace stan {
namespace math {

/**
 * Integrator interface for CVODES' ODE solvers (Adams & BDF
 * methods).
 *
 * @tparam Lmm ID of ODE solver (1: ADAMS, 2: BDF)
 */
template <int Lmm, typename F, typename T_initial, typename... T_Args>
class cvodes_integrator {
  using T_Return = return_type_t<T_initial, T_Args...>;

  const F& f_;
  const std::vector<T_initial>& y0_;
  double t0_;
  const std::vector<double>& ts_;
  std::tuple<const T_Args&...> args_tuple_;
  std::tuple<decltype(value_of(T_Args()))...> value_of_args_tuple_;
  const size_t N_;
  std::ostream* msgs_;
  double relative_tolerance_;
  double absolute_tolerance_;
  long int max_num_steps_;

  const size_t y0_vars_;
  const size_t args_vars_;

  coupled_ode_system<F, T_initial, T_Args...> coupled_ode_;

  std::vector<double> coupled_state_;
  N_Vector nv_state_;
  N_Vector* nv_state_sens_;
  SUNMatrix A_;
  SUNLinearSolver LS_;

  /**
   * Implements the function of type CVRhsFn which is the user-defined
   * ODE RHS passed to CVODES.
   */
  static int cv_rhs(realtype t, N_Vector y, N_Vector ydot, void* user_data) {
    const cvodes_integrator* integrator
        = static_cast<const cvodes_integrator*>(user_data);
    integrator->rhs(t, NV_DATA_S(y), NV_DATA_S(ydot));
    return 0;
  }

  /**
   * Implements the function of type CVSensRhsFn which is the
   * RHS of the sensitivity ODE system.
   */
  static int cv_rhs_sens(int Ns, realtype t, N_Vector y, N_Vector ydot,
                         N_Vector* yS, N_Vector* ySdot, void* user_data,
                         N_Vector tmp1, N_Vector tmp2) {
    const cvodes_integrator* integrator
        = static_cast<const cvodes_integrator*>(user_data);
    integrator->rhs_sens(t, NV_DATA_S(y), yS, ySdot);
    return 0;
  }

  /**
   * Implements the function of type CVDlsJacFn which is the
   * user-defined callback for CVODES to calculate the jacobian of the
   * ode_rhs wrt to the states y. The jacobian is stored in column
   * major format.
   */
  static int cv_jacobian_states(realtype t, N_Vector y, N_Vector fy,
                                SUNMatrix J, void* user_data, N_Vector tmp1,
                                N_Vector tmp2, N_Vector tmp3) {
    const cvodes_integrator* integrator
        = static_cast<const cvodes_integrator*>(user_data);
    integrator->jacobian_states(t, NV_DATA_S(y), J);
    return 0;
  }

  /**
   * Calculates the ODE RHS, dy_dt, using the user-supplied functor at
   * the given time t and state y.
   */
  inline void rhs(double t, const double y[], double dy_dt[]) const {
    const std::vector<double> y_vec(y, y + N_);
    std::vector<double> dy_dt_vec
        = apply([&](auto&&... args) { return f_(t, y_vec, msgs_, args...); },
                value_of_args_tuple_);
    check_size_match("cvodes_integrator::rhs", "dy_dt", dy_dt_vec.size(),
                     "states", N_);
    std::move(dy_dt_vec.begin(), dy_dt_vec.end(), dy_dt);
  }

  /**
   * Calculates the jacobian of the ODE RHS wrt to its states y at the
   * given time-point t and state y.
   */
  inline void jacobian_states(double t, const double y[], SUNMatrix J) const {
    Eigen::VectorXd fy;
    Eigen::MatrixXd Jfy;

    auto f_wrapped = [&](const Eigen::Matrix<var, Eigen::Dynamic, 1>& y) {
      return to_vector(apply(
          [&](auto&&... args) { return f_(t, to_array_1d(y), msgs_, args...); },
          value_of_args_tuple_));
    };

    jacobian(f_wrapped, Eigen::Map<const Eigen::VectorXd>(y, N_), fy, Jfy);

    for (size_t j = 0; j < Jfy.cols(); ++j) {
      for (size_t i = 0; i < Jfy.rows(); ++i) {
        SM_ELEMENT_D(J, i, j) = Jfy(i, j);
      }
    }
  }

  /**
   * Calculates the RHS of the sensitivity ODE system which
   * corresponds to the coupled ode system from which the first N
   * states are omitted, since the first N states are the ODE RHS
   * which CVODES separates from the main ODE RHS.
   */
  inline void rhs_sens(double t, const double y[], N_Vector* yS,
                       N_Vector* ySdot) const {
    std::vector<double> z(coupled_state_.size());
    std::vector<double>&& dz_dt = std::vector<double>(coupled_state_.size());
    std::copy(y, y + N_, z.begin());
    for (std::size_t s = 0; s < y0_vars_ + args_vars_; s++) {
      std::copy(NV_DATA_S(yS[s]), NV_DATA_S(yS[s]) + N_,
                z.begin() + (s + 1) * N_);
    }
    coupled_ode_(z, dz_dt, t);
    for (std::size_t s = 0; s < y0_vars_ + args_vars_; s++) {
      std::move(dz_dt.begin() + (s + 1) * N_, dz_dt.begin() + (s + 2) * N_,
                NV_DATA_S(ySdot[s]));
    }
  }

 public:
  cvodes_integrator(const F& f, const std::vector<T_initial>& y0, double t0,
                    const std::vector<double>& ts, double relative_tolerance,
                    double absolute_tolerance, long int max_num_steps,
                    std::ostream* msgs, const T_Args&... args)
      : f_(f),
        y0_(y0),
        t0_(t0),
        ts_(ts),
        args_tuple_(args...),
        value_of_args_tuple_(value_of(args)...),
        N_(y0.size()),
        msgs_(msgs),
        relative_tolerance_(relative_tolerance),
        absolute_tolerance_(absolute_tolerance),
        max_num_steps_(max_num_steps),
        y0_vars_(count_vars(y0_)),
        args_vars_(count_vars(args...)),
        coupled_ode_(f, y0, msgs, args...),
        coupled_state_(coupled_ode_.initial_state()) {
    using initial_var = stan::is_var<T_initial>;

    const char* fun = "cvodes_integrator::integrate";

    check_finite(fun, "initial state", y0_);
    check_finite(fun, "initial time", t0_);
    check_finite(fun, "times", ts_);

    // Code from: https://stackoverflow.com/a/17340003 . Should probably do
    // something better
    apply(
        [&](auto&&... args) {
          std::vector<int> unused_temp{
              0, (check_finite(fun, "ode parameters and data", args), 0)...};
        },
        args_tuple_);

    check_nonzero_size(fun, "times", ts_);
    check_nonzero_size(fun, "initial state", y0_);
    check_ordered(fun, "times", ts_);
    check_less(fun, "initial time", t0_, ts_[0]);
    check_positive_finite(fun, "relative_tolerance", relative_tolerance_);
    check_positive_finite(fun, "absolute_tolerance", absolute_tolerance_);
    check_positive(fun, "max_num_steps", max_num_steps_);

    nv_state_ = N_VMake_Serial(N_, &coupled_state_[0]);
    nv_state_sens_ = nullptr;
    A_ = SUNDenseMatrix(N_, N_);
    LS_ = SUNDenseLinearSolver(nv_state_, A_);

    if (y0_vars_ + args_vars_ > 0) {
      nv_state_sens_
          = N_VCloneVectorArrayEmpty_Serial(y0_vars_ + args_vars_, nv_state_);
      for (std::size_t i = 0; i < y0_vars_ + args_vars_; i++) {
        NV_DATA_S(nv_state_sens_[i]) = &coupled_state_[N_] + i * N_;
      }
    }
  }

  ~cvodes_integrator() {
    SUNLinSolFree(LS_);
    SUNMatDestroy(A_);
    N_VDestroy_Serial(nv_state_);
    if (y0_vars_ + args_vars_ > 0) {
      N_VDestroyVectorArray_Serial(nv_state_sens_, y0_vars_ + args_vars_);
    }
  }

  /**
   * Return the solutions for the specified system of ordinary
   * differential equations given the specified initial state,
   * initial times, times of desired solution, and parameters and
   * data, writing error and warning messages to the specified
   * stream.
   *
   * This function is templated to allow the initials to be
   * either data or autodiff variables and the parameters to be data
   * or autodiff variables.  The autodiff-based implementation for
   * reverse-mode are defined in namespace <code>stan::math</code>
   * and may be invoked via argument-dependent lookup by including
   * their headers.
   *
   * The solver used is based on the backward differentiation
   * formula which is an implicit numerical integration scheme
   * appropriate for stiff ODE systems.
   *
   * @tparam F type of ODE system function.
   * @tparam T_initial type of scalars for initial values.
   * @tparam T_param type of scalars for parameters.
   * @tparam T_t0 type of scalar of initial time point.
   * @tparam T_ts type of time-points where ODE solution is returned.
   *
   * @param[in] f functor for the base ordinary differential equation.
   * @param[in] y0 initial state.
   * @param[in] t0 initial time.
   * @param[in] ts times of the desired solutions, in strictly
   * increasing order, all greater than the initial time.
   * @param[in] theta parameter vector for the ODE.
   * @param[in] x continuous data vector for the ODE.
   * @param[in] x_int integer data vector for the ODE.
   * @param[in, out] msgs the print stream for warning messages.
   * @param[in] relative_tolerance relative tolerance passed to CVODE.
   * @param[in] absolute_tolerance absolute tolerance passed to CVODE.
   * @param[in] max_num_steps maximal number of admissable steps
   * between time-points
   * @return a vector of states, each state being a vector of the
   * same size as the state variable, corresponding to a time in ts.
   */
  std::vector<std::vector<T_Return>> integrate() {  // NOLINT(runtime/int)
    std::vector<std::vector<T_Return>> y;

    const double t0_dbl = value_of(t0_);
    const std::vector<double> ts_dbl = value_of(ts_);

    void* cvodes_mem = CVodeCreate(Lmm);
    if (cvodes_mem == nullptr) {
      throw std::runtime_error("CVodeCreate failed to allocate memory");
    }

    try {
      check_flag_sundials(
          CVodeInit(cvodes_mem, &cvodes_integrator::cv_rhs, t0_dbl, nv_state_),
          "CVodeInit");

      // Assign pointer to this as user data
      check_flag_sundials(
          CVodeSetUserData(cvodes_mem, reinterpret_cast<void*>(this)),
          "CVodeSetUserData");

      cvodes_set_options(cvodes_mem, relative_tolerance_, absolute_tolerance_,
                         max_num_steps_);

      // for the stiff solvers we need to reserve additional memory
      // and provide a Jacobian function call. new API since 3.0.0:
      // create matrix object and linear solver object; resource
      // (de-)allocation is handled in the cvodes_ode_data
      check_flag_sundials(CVodeSetLinearSolver(cvodes_mem, LS_, A_),
                          "CVodeSetLinearSolver");
      check_flag_sundials(
          CVodeSetJacFn(cvodes_mem, &cvodes_integrator::cv_jacobian_states),
          "CVodeSetJacFn");

      // initialize forward sensitivity system of CVODES as needed
      if (y0_vars_ + args_vars_ > 0) {
        check_flag_sundials(
            CVodeSensInit(cvodes_mem, static_cast<int>(y0_vars_ + args_vars_),
                          CV_STAGGERED, &cvodes_integrator::cv_rhs_sens,
                          nv_state_sens_),
            "CVodeSensInit");

        check_flag_sundials(CVodeSensEEtolerances(cvodes_mem),
                            "CVodeSensEEtolerances");
      }

      double t_init = t0_dbl;
      for (size_t n = 0; n < ts_.size(); ++n) {
        double t_final = ts_dbl[n];

        if (t_final != t_init) {
          check_flag_sundials(
              CVode(cvodes_mem, t_final, nv_state_, &t_init, CV_NORMAL),
              "CVode");
        }

        if (y0_vars_ + args_vars_ > 0) {
          check_flag_sundials(CVodeGetSens(cvodes_mem, &t_init, nv_state_sens_),
                              "CVodeGetSens");
        }

        y.emplace_back(apply(
            [&](auto&&... args) {
              return ode_store_sensitivities(coupled_state_, y0_, args...);
            },
            args_tuple_));

        t_init = t_final;
      }
    } catch (const std::exception& e) {
      CVodeFree(&cvodes_mem);
      throw;
    }

    CVodeFree(&cvodes_mem);

    return y;
  }
};  // cvodes integrator

}  // namespace math
}  // namespace stan
#endif
