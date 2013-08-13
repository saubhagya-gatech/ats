/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/*
  The elevation evaluator gets the surface elevation, slope, and updates pres + elev.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#include "elevation_evaluator.hh"

namespace Amanzi {
namespace Flow {
namespace FlowRelations {

ElevationEvaluator::ElevationEvaluator(Teuchos::ParameterList& plist) :
    SecondaryVariablesFieldEvaluator(plist),
    updated_once_(false), 
    dynamic_mesh_(false) {
  my_keys_.push_back(plist_.get<std::string>("elevation key", "elevation"));
  my_keys_.push_back(plist_.get<std::string>("slope magnitude key", "slope_magnitude"));
  setLinePrefix(my_keys_[0]+std::string(" evaluator"));

  // If the mesh changes dynamically (e.g. due to the presence of a deformation
  // pk, then we must make sure that elevation is recomputed every time the 
  // mesh has been deformed. The indicator for the mesh deformation event is the 
  // the deformation field.
  dynamic_mesh_ = plist_.get<bool>("dynamic mesh",false);
  if (dynamic_mesh_) dependencies_.insert("deformation");
}

void ElevationEvaluator::EvaluateField_(const Teuchos::Ptr<State>& S,
        const std::vector<Teuchos::Ptr<CompositeVector> >& results) {
  EvaluateElevationAndSlope_(S, results);

  // If boundary faces are requested, grab the slopes on the internal cell
  Teuchos::Ptr<CompositeVector> slope = results[1];

  if (slope->has_component("boundary_face")) {
    const Epetra_Map& vandelay_map = slope->mesh()->exterior_face_epetra_map();
    const Epetra_Map& face_map = slope->mesh()->face_epetra_map(false);
    Epetra_MultiVector& slope_bf = *slope->ViewComponent("boundary_face",false);
    const Epetra_MultiVector& slope_c = *slope->ViewComponent("cell",false);

    // calculate boundary face values
    AmanziMesh::Entity_ID_List cells;
    int nbfaces = slope_bf.MyLength();
    for (int bf=0; bf!=nbfaces; ++bf) {
      // given a boundary face, we need the internal cell to choose the right WRM
      AmanziMesh::Entity_ID f = face_map.LID(vandelay_map.GID(bf));
      slope->mesh()->face_get_cells(f, AmanziMesh::USED, &cells);
      ASSERT(cells.size() == 1);

      slope_bf[0][bf] = slope_c[0][cells[0]];
    }
  }
}

// This is hopefully never called?
void ElevationEvaluator::EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
        Key wrt_key, const std::vector<Teuchos::Ptr<CompositeVector> >& results) {
  ASSERT(0);
}

// Custom EnsureCompatibility forces this to be updated once.
bool ElevationEvaluator::HasFieldChanged(const Teuchos::Ptr<State>& S,
                                         Key request) {
  bool changed = SecondaryVariablesFieldEvaluator::HasFieldChanged(S,request);
  if (!updated_once_) {
    UpdateField_(S);
    updated_once_ = true;
    return true;
  }
  return changed;
}


void ElevationEvaluator::EnsureCompatibility(const Teuchos::Ptr<State>& S) {
  // Ensure my field exists, and claim ownership.
  for (std::vector<Key>::const_iterator my_key=my_keys_.begin();
       my_key!=my_keys_.end(); ++my_key) {
    Teuchos::RCP<CompositeVectorFactory> my_fac = S->RequireField(*my_key, *my_key);

    // check plist for vis or checkpointing control
    bool io_my_key = plist_.get<bool>(std::string("visualize ")+*my_key, true);
    S->GetField(*my_key, *my_key)->set_io_vis(io_my_key);
    bool checkpoint_my_key = plist_.get<bool>(std::string("checkpoint ")+*my_key, false);
    S->GetField(*my_key, *my_key)->set_io_checkpoint(checkpoint_my_key);
  }
};



} //namespace
} //namespace
} //namespace
