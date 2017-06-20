/*
  The richards energy model is an algebraic model with dependencies.

  Generated via evaluator_generator with:
Richards energy: the standard form as a function of liquid saturation and specific internal energy.
    
  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#include "Teuchos_ParameterList.hpp"
#include "dbc.hh"
#include "richards_energy_model.hh"

namespace Amanzi {
namespace Energy {
namespace Relations {

// Constructor from ParameterList
RichardsEnergyModel::RichardsEnergyModel(Teuchos::ParameterList& plist)
{
  InitializeFromPlist_(plist);
}


// Initialize parameters
void
RichardsEnergyModel::InitializeFromPlist_(Teuchos::ParameterList& plist)
{

}


// main method
double
RichardsEnergyModel::Energy(double phi, double sl, double nl, double ul, double rho_r, double ur, double cv) const
{
  return cv*(nl*phi*sl*ul + rho_r*ur*(-phi + 1));
}

double
RichardsEnergyModel::DEnergyDPorosity(double phi, double sl, double nl, double ul, double rho_r, double ur, double cv) const
{
  return cv*(nl*sl*ul - rho_r*ur);
}

double
RichardsEnergyModel::DEnergyDSaturationLiquid(double phi, double sl, double nl, double ul, double rho_r, double ur, double cv) const
{
  return cv*nl*phi*ul;
}

double
RichardsEnergyModel::DEnergyDMolarDensityLiquid(double phi, double sl, double nl, double ul, double rho_r, double ur, double cv) const
{
  return cv*phi*sl*ul;
}

double
RichardsEnergyModel::DEnergyDInternalEnergyLiquid(double phi, double sl, double nl, double ul, double rho_r, double ur, double cv) const
{
  return cv*nl*phi*sl;
}

double
RichardsEnergyModel::DEnergyDDensityRock(double phi, double sl, double nl, double ul, double rho_r, double ur, double cv) const
{
  return cv*ur*(-phi + 1);
}

double
RichardsEnergyModel::DEnergyDInternalEnergyRock(double phi, double sl, double nl, double ul, double rho_r, double ur, double cv) const
{
  return cv*rho_r*(-phi + 1);
}

double
RichardsEnergyModel::DEnergyDCellVolume(double phi, double sl, double nl, double ul, double rho_r, double ur, double cv) const
{
  return nl*phi*sl*ul + rho_r*ur*(-phi + 1);
}

} //namespace
} //namespace
} //namespace
  