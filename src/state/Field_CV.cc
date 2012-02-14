/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */
/* -------------------------------------------------------------------------
ATS

License: see $ATS_DIR/COPYRIGHT
Author: Ethan Coon

Implementation for a Field.  Field is not intended so much to hide implementation
of data as to restrict write access to data.  It freely passes out pointers to
its private data, but only passes out read-only const pointers unless you have
the secret password (AKA the name of the process kernel that owns the data).

Field also stores some basic metadata for Vis, checkpointing, etc.
------------------------------------------------------------------------- */

#include "errors.hh"
#include "CompositeVector.hh"
#include "Field.hh"
#include "Field_CV.hh"

namespace Amanzi {

Field_CV::Field_CV(std::string fieldname, std::string owner) :
    Field::Field(fieldname, owner), data_() {
  type_ = VECTOR_FIELD;
};

Field_CV::Field_CV(std::string fieldname, std::string owner,
                           Teuchos::RCP<CompositeVector>& data) :
    Field::Field(fieldname, owner), data_(data) {
  type_ = VECTOR_FIELD;
};

// copy constructor:
Field_CV::Field_CV(const Field_CV& other) :
    Field::Field(other.fieldname_, other.owner_) {
  io_restart_ = other.io_restart_;
  io_vis_ = other.io_vis_;
  initialized_ = other.initialized_;
  type_ = other.type_;
  data_ = Teuchos::rcp(new CompositeVector(*other.data_));
};

// Virtual copy constructor
Teuchos::RCP<Field> Field_CV::Clone() const {
  return Teuchos::rcp(new Field_CV(*this));
}

// Virtual copy constructor with non-empty name
Teuchos::RCP<Field> Field_CV::Clone(std::string fieldname) const {
  Teuchos::RCP<Field_CV> other = Teuchos::rcp(new Field_CV(*this));
  other->fieldname_ = fieldname;
  return other;
};

// Virtual copy constructor with non-empty name
Teuchos::RCP<Field> Field_CV::Clone(std::string fieldname, std::string owner) const {
  Teuchos::RCP<Field_CV> other = Teuchos::rcp(new Field_CV(*this));
  other->fieldname_ = fieldname;
  other->owner_ = owner;
  return other;
};

// write-access to the data
Teuchos::RCP<CompositeVector> Field_CV::GetFieldData(std::string pk_name) {
  assert_owner_or_die_(pk_name);
  return data_;
};

// Overwrite data by pointer, not copy
void Field_CV::SetData(std::string pk_name, Teuchos::RCP<CompositeVector>& data) {
  assert_owner_or_die_(pk_name);
  data_ = data;
};

void Field_CV::SetData(std::string pk_name, const CompositeVector& data) {
  assert_owner_or_die_(pk_name);
  *data_ = data;
};

void Field_CV::Initialize(Teuchos::ParameterList& plist) {
  const std::vector<std::string> component_names = data_->names();
  const std::vector<int> num_dofs = data_->num_dofs();
  const int num_components = data_->num_components();
  const std::vector< std::vector<std::string> > subfield_names = data_->subfield_names();

  std::vector< std::vector<double> > vals(num_components);
  for (unsigned int i = 0; i != num_components; ++i) {
    vals[i].resize(num_dofs[i]);
  }

  // try to set the field on a per-subfield basis
  bool got_them_all = true;
  for (unsigned int lcv_component=0; lcv_component != num_components; ++lcv_component) {
    for (unsigned int lcv_dof = 0; lcv_dof != num_dofs[lcv_component]; ++lcv_dof) {
      // attempt to pick out constant values for the field
      std::string subname = subfield_names[lcv_component][lcv_dof];
      if (subname != std::string("") &&
          plist.isParameter("Constant "+subname)) {
        vals[lcv_component][lcv_dof] = plist.get<double>("Constant "+subname);
        std::cout << "  got value:" << subname << "=" <<
        vals[lcv_component][lcv_dof] << std::endl;
      } else {
        got_them_all = false;
        break;
      }
    }
  }
  if (got_them_all) {
    std::cout << "got them all, assigning" << std::endl;
    for (unsigned int lcv_component=0; lcv_component != num_components; ++lcv_component) {
      data_->PutScalar(component_names[lcv_component], vals[lcv_component]);
    }
    set_initialized();
  }

  // try to set the field on a per-subfield, per-mesh set basis
  if (plist.isParameter("Number of mesh blocks")) {
    int num_blocks = plist.get<int>("Number of mesh blocks");

    bool got_a_block = false;
    for (int nb=1; nb<=num_blocks; nb++) {
      std::stringstream pname;
      pname << "Mesh block " << nb;
      Teuchos::ParameterList sublist = plist.sublist(pname.str());
      int mesh_block_id = sublist.get<int>("Mesh block ID");

      got_them_all = true;
      for (unsigned int lcv_component=0; lcv_component != num_components; ++lcv_component) {
        for (unsigned int lcv_dof = 0; lcv_dof != num_dofs[lcv_component]; ++lcv_dof) {
          // attempt to pick out constant values for the field
          std::string subname = subfield_names[lcv_component][lcv_dof];
          if (subname != std::string("") &&
              plist.isParameter("Constant "+subname)) {
            vals[lcv_component][lcv_dof] = plist.get<double>("Constant "+subname);
            std::cout << "  got value:" << subname << "=" <<
              vals[lcv_component][lcv_dof] << std::endl;
          } else {
            got_them_all = false;
            break;
          }
        }
      }

      if (got_a_block && !got_them_all) {
        std::stringstream messagestream;
        messagestream << "Field " << fieldname_
                      << "initialized at least one, but not all mesh block.";
        Errors::Message message(messagestream.str());
        Exceptions::amanzi_throw(message);
      }

      if (got_them_all) {
        got_a_block = true;
        std::cout << "got them all, assigning" << std::endl;
        for (unsigned int lcv_component=0; lcv_component != num_components; ++lcv_component) {
          data_->PutScalar(component_names[lcv_component], mesh_block_id, vals[lcv_component]);
        }
      }
      set_initialized();
    }
  }
};

void Field_CV::WriteVis(Amanzi::Vis& vis) {
  if (io_vis_) {
    const std::vector< std::vector<std::string> > subfield_names = data_->subfield_names();
    const std::vector<std::string> names = data_->names();
    for (unsigned int i = 0; i != data_->num_components(); ++i) {
      vis.write_vector(*data_->ViewComponent(names[i], false), subfield_names[i]);
    }
  }
};

} // namespace
