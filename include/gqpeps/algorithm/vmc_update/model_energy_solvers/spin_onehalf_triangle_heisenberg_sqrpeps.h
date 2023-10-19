/*
* Author: Hao-Xin Wang<wanghaoxin1996@gmail.com>
* Creation Date: 2023-09-28
*
* Description: GraceQ/VMC-PEPS project. Model Energy Solver for spin-1/2 Triangle Heisenberg model on square PEPS
*/

#ifndef GRACEQ_VMC_PEPS_SPIN_ONEHALF_TRIANGLE_HEISENBERG_SQRPEPS_H
#define GRACEQ_VMC_PEPS_SPIN_ONEHALF_TRIANGLE_HEISENBERG_SQRPEPS_H


#include "gqpeps/algorithm/vmc_update/model_energy_solver.h"    //ModelEnergySolver


namespace gqpeps {
using namespace gqten;


template<typename TenElemT, typename QNT>
class SpinOneHalfTriHeisenbergSqrPEPS : public ModelEnergySolver<TenElemT, QNT> {
  using SITPS = SplitIndexTPS<TenElemT, QNT>;
 public:
  SpinOneHalfTriHeisenbergSqrPEPS(void) = default;

  TenElemT CalEnergyAndHoles(
      const SITPS *sitps,
      TPSSample<TenElemT, QNT> *tps_sample,
      TensorNetwork2D<TenElemT, QNT> &hole_res
  ) override;

 private:
};

template<typename TenElemT, typename QNT>
TenElemT SpinOneHalfTriHeisenbergSqrPEPS<TenElemT, QNT>::CalEnergyAndHoles(const SITPS *split_index_tps,
                                                                           TPSSample<TenElemT, QNT> *tps_sample,
                                                                           TensorNetwork2D<TenElemT, QNT> &hole_res) {
  TenElemT e(0); // energy in J1 and J2 bond respectively
  TensorNetwork2D<TenElemT, QNT> &tn = tps_sample->tn;
  const Configuration &config = tps_sample->config;
  TenElemT inv_psi = 1.0 / (tps_sample->amplitude);
  tn.GenerateBMPSApproach(UP);
  for (size_t row = 0; row < tn.rows(); row++) {
    tn.InitBTen(LEFT, row);
    tn.GrowFullBTen(RIGHT, row, 1, true);
    for (size_t col = 0; col < tn.cols(); col++) {
      const SiteIdx site1 = {row, col};
      //Calculate the holes
      hole_res(site1) = Dag(tn.PunchHole(site1, HORIZONTAL));
      if (col < tn.cols() - 1) {
        //Calculate horizontal bond energy contribution
        const SiteIdx site2 = {row, col + 1};
        if (config(site1) == config(site2)) {
          e += 0.25;
        } else {
          TenElemT psi_ex = tn.ReplaceNNSiteTrace(site1, site2, HORIZONTAL,
                                                  (*split_index_tps)(site1)[config(site2)],
                                                  (*split_index_tps)(site2)[config(site1)]);
          e += (-0.25 + psi_ex * inv_psi * 0.5);
        }
        tn.BTenMoveStep(RIGHT);
      }
    }
    if (row < tn.rows() - 1) {
      //calculate J2 energy
      tn.InitBTen2(LEFT, row);
      tn.GrowFullBTen2(RIGHT, row, 2, true);

      for (size_t col = 0; col < tn.cols() - 1; col++) {
        //Calculate diagonal energy contribution
        SiteIdx site1 = {row + 1, col}; //left-down
        SiteIdx site2 = {row, col + 1}; //right-up
        if (config(site1) == config(site2)) {
          e += 0.25;
        } else {
          TenElemT psi_ex = tn.ReplaceNNNSiteTrace({row, col},
                                                   LEFTDOWN_TO_RIGHTUP,
                                                   HORIZONTAL,
                                                   (*split_index_tps)(site1)[config(site2)],  //the tensor at left
                                                   (*split_index_tps)(site2)[config(site1)]);
          e += (-0.25 + psi_ex * inv_psi * 0.5);
        }
        tn.BTen2MoveStep(RIGHT, row);
      }
      tn.BMPSMoveStep(DOWN);
    }
  }

  //Calculate vertical bond energy contribution
  tn.GenerateBMPSApproach(LEFT);
  for (size_t col = 0; col < tn.cols(); col++) {
    tn.InitBTen(UP, col);
    tn.GrowFullBTen(DOWN, col, 2, true);
    for (size_t row = 0; row < tn.rows() - 1; row++) {
      const SiteIdx site1 = {row, col};
      const SiteIdx site2 = {row + 1, col};
      if (config(site1) == config(site2)) {
        e += 0.25;
      } else {
        TenElemT psi_ex = tn.ReplaceNNSiteTrace(site1, site2, VERTICAL,
                                                (*split_index_tps)(site1)[config(site2)],
                                                (*split_index_tps)(site2)[config(site1)]);
        e += (-0.25 + psi_ex * inv_psi * 0.5);
      }
      if (row < tn.rows() - 2) {
        tn.BTenMoveStep(DOWN);
      }
    }
    if (col < tn.cols() - 1) {
      tn.BMPSMoveStep(RIGHT);
    }
  }
  return e;
}

}//gqpeps



#endif //GRACEQ_VMC_PEPS_SPIN_ONEHALF_TRIANGLE_HEISENBERG_SQRPEPS_H