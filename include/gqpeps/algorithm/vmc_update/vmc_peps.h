// SPDX-License-Identifier: LGPL-3.0-only

/*
* Author: Hao-Xin Wang<wanghaoxin1996@gmail.com>
* Creation Date: 2023-07-25
*
* Description: GraceQ/VMC-PEPS project. The generic PEPS class, implementation.
*/

#ifndef GQPEPS_ALGORITHM_VMC_UPDATE_VMC_UPDATE_H
#define GQPEPS_ALGORITHM_VMC_UPDATE_VMC_UPDATE_H

#include <random>                                   // default_random_engine
#include "gqpeps/two_dim_tn/tps/tps.h"              // TPS
#include "gqpeps/two_dim_tn/tps/split_index_tps.h"  //SplitIndexTPS

#include "gqpeps/algorithm/vmc_update/model_energy_solver.h"
#include "gqpeps/algorithm/vmc_update/tps_sample.h"

#include "boost/mpi.hpp"                            //boost::mpi

namespace gqpeps {
using namespace gqten;

const size_t kMasterProc = 0;

std::default_random_engine random_engine;

struct VMCOptimizePara {

  TruncatePara bmps_trunc_para; // Truncation Error and bond dimensionts for compressing boundary MPS

  size_t mc_samples;
  size_t mc_warm_up_sweeps;

  // e.g. In spin model, how many spin up sites and how many spin down sites.
  std::vector<size_t> occupancy_num;

  std::vector<double> step_lens;

  VMCOptimizePara(double truncErr, size_t Dmin, size_t Dmax, size_t samples, size_t warm_up_sweeps,
                  const std::vector<size_t> &occupancy, const std::vector<double> &step_len)
      : bmps_trunc_para(Dmin, Dmax, truncErr), mc_samples(samples),
        mc_warm_up_sweeps(warm_up_sweeps), occupancy_num(occupancy), step_lens(step_len) {}

  operator TruncatePara() const {
    return bmps_trunc_para;
  }
};

template<typename TenElemT, typename QNT, typename EnergySolver>
class VMCPEPSExecutor : public Executor {
 public:
  using Tensor = GQTensor<TenElemT, QNT>;
  using TPST = TPS<TenElemT, QNT>;
  using SITPST = SplitIndexTPS<TenElemT, QNT>;
  using IndexT = Index<QNT>;

  VMCPEPSExecutor(const VMCOptimizePara &optimize_para,
                  const TPST &tps_init,
                  const boost::mpi::communicator &world);

  VMCPEPSExecutor(const VMCOptimizePara &optimize_para,
                  const SITPST &sitpst_init,
                  const boost::mpi::communicator &world);


  void Execute(void) override;

  void LoadTenData(const std::string &tps_path = kTpsPath);

  void DumpTenData(const std::string &tps_path = kTpsPath, const bool release_mem = false);

  VMCOptimizePara optimize_para;

 private:

  void WarmUp_(void);

  void OptimizeTPS_(void);

  void Measure_(void);

  void MCSweepSequentially_(void);

  void MCUpdateNNSite_(const SiteIdx &site_a, BondOrientation dir);

  void SampleEnergyAndHols_(void);

  void ClearEnergyAndHoleSamples_(void);

  //return the grad;
  SITPST GatherStatisticEnergyAndGrad_(void);

  void GradUpdateTPS_(const SITPST &grad, const double step_len);

  boost::mpi::communicator world_;

  size_t lx_; //cols
  size_t ly_; //rows

  SITPST split_index_tps_;

  TPSSample<TenElemT, QNT> tps_sample_;

  std::uniform_real_distribution<double> u_double_;

  std::vector<TenElemT> energy_samples_;
  ///<outside vector indices corresponding to the local hilbert space basis
  DuoMatrix<std::vector<std::vector<Tensor *> >> gten_samples_;
  DuoMatrix<std::vector<std::vector<Tensor *> >> g_times_energy_samples_;

  SITPST grad_;

  std::vector<TenElemT> energy_trajectory_;
  std::vector<TenElemT> energy_error_traj_;
  std::vector<double> grad_norm_;

  EnergySolver energy_solver_;
};




}//gqpeps;

#include "gqpeps/algorithm/vmc_update/vmc_peps_impl.h"

#endif //GQPEPS_ALGORITHM_VMC_UPDATE_VMC_UPDATE_H