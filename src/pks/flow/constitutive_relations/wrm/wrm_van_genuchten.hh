/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/*
  Amanzi needs to fix its WRM interface... the current lack of factory or
  common interface is stupid.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#ifndef _FLOWRELATIONS_WRM_VAN_GENUCHTEN_
#define _FLOWRELATIONS_WRM_VAN_GENUCHTEN_

#include "Teuchos_ParameterList.hpp"
#include "wrm.hh"
#include "factory.hh"

namespace Amanzi {
namespace Flow {
namespace FlowRelations {

class WRMVanGenuchten : public WRM {

public:
  explicit WRMVanGenuchten(Teuchos::ParameterList& plist);

  // required methods from the base class
  double k_relative(double pc);
  double d_k_relative(double pc);
  double saturation(double pc);
  double d_saturation(double pc);
  double capillaryPressure(double saturation);
  double d_capillaryPressure(double saturation);
  double residualSaturation() { return sr_; }

 private:
  void InitializeFromPlist_();

  Teuchos::ParameterList& plist_;

  double m_;  // van Genuchten parameters: m, n, alpha
  double n_;
  double l_;
  double alpha_;
  double sr_;  // van Genuchten residual saturation

  int function_;
  double pc0_;  // regularization threshold (usually 0 to 500 Pa)
  double a_, b_, factor_dSdPc_;  // frequently used constant

  static Utils::RegisteredFactory<WRM,WRMVanGenuchten> factory_;
};

} //namespace
} //namespace
} //namespace

#endif