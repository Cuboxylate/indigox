/** @file molecule.cpp
 *  @brief Implementation of Molecule class
 *  @author Ivan Welsh
 *  @date 6 January 2018
 *  @lastmodify 8 January 2018
 *  @version 0.1
 *  @copyright The MIT License
 */

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>

#include "indigox/classes/atom.hpp"
#include "indigox/classes/bond.hpp"
#include "indigox/classes/molecular_graph.hpp"
#include "indigox/classes/molecule.hpp"
#include "indigox/classes/periodictable.hpp"
#include "indigox/utils/counter.hpp"
#include "indigox/utils/options.hpp"


using namespace indigox;

/*
 * Molecule implementation
 */

// Initalisation methods
IXMolecule::IXMolecule()
: name_("None"), graph_(new _MolecularGraph()), elnopt_(new ElectronOpt()){ }

/** @param name the name to call the molecule. */
IXMolecule::IXMolecule(std::string name) : IXMolecule() {
  name_ = name;
}

IXMolecule::~IXMolecule() { }

// Data retrival methods

/** @param i the atom index to get
 *  @details Atom indicies are unstable. Use of this method is not recommended
 *  unless you know what you're doing.
 */
Atom IXMolecule::GetAtomIndex(uid_t i) {
  Atom atm = Atom();
  auto it = idx_to_atom_.find(i);
  if (it != idx_to_atom_.end() && !(it->second.expired())
      && it->second.lock()->GetIndex() == i) {
    atm = it->second.lock();
  } else {
    idx_to_atom_.clear();
    for (Atom a : atoms_) {
      if (a->GetIndex() == i) atm = a;
      idx_to_atom_.emplace(a->GetIndex(), a);
    }
  }
  return atm;
}

/** @param i the unique id of the atom to get
 *  @details Returns the atom in the molecule with the given unique id, if it exists
 */
Atom IXMolecule::GetAtomUniqueID(uid_t i) {
  for (Atom atm : atoms_) {
    if (atm->GetUniqueID() == i) return atm;
  }
  return Atom();
}

/** @param i the bond index to get
 *  @details Bond indicies are unstable. Use of this method is not recommended
 *  unless you know what you're doing.
 */
Bond IXMolecule::GetBondIndex(uid_t i) {
  Bond bnd = Bond();
  auto it = idx_to_bond_.find(i);
  if (it != idx_to_bond_.end() && !(it->second.expired())
      && it->second.lock()->GetIndex() == i) {
    bnd = it->second.lock();
  } else {
    idx_to_bond_.clear();
    for (Bond b : bonds_) {
      if (b->GetIndex() == i) bnd = b;
      idx_to_bond_.emplace(b->GetIndex(), b);
    }
  }
  return bnd;
}

/** @param a the source atom of the bond to get.
 *  @param b the target atom of the bonds to get.
 *  @details Atoms should both be in the molecule. Returns a null bond if no
 *  such bond exists.
 */
Bond IXMolecule::GetBond(Atom a, Atom b) const {
  /// @todo Logging system calls for fail reasons
  Bond bnd = Bond();
  if (a->GetMolecule()->GetUniqueID() != GetUniqueID()) return bnd;
  if (b->GetMolecule()->GetUniqueID() != GetUniqueID()) return bnd;
  for (auto it = a->GetBondIters(); it.first != it.second; ++it.first) {
    Bond tmp = it.first->lock();
    if (tmp->GetSourceAtom() == a && tmp->GetTargetAtom() == b) return tmp;
    if (tmp->GetSourceAtom() == b && tmp->GetTargetAtom() == a) return tmp;
  }
  return bnd;
}

/** @details Generates the molecular formula of the molecule based on its atoms.
 *  If any atom does not have an element set, an \"Xx\" symbol is added to the
 *  formula.
 */
std::string IXMolecule::GetFormula() const {
  std::map<std::string, size_t> e_count;
  for (Atom atm : atoms_) {
    Element elem = atm->GetElement();
    if (!elem) e_count["Xx"]++;
    else e_count[elem->GetSymbol()]++;
  }
  
  std::stringstream ss;
  if (e_count["C"]) ss << "C";
  if (e_count["C"] > 1) ss << e_count["C"];
  if (e_count["H"]) ss << "H";
  if (e_count["H"] > 1) ss << e_count["H"];
  for (auto& e : e_count) {
    if (e.first != "C" && e.first != "H") {
      ss << e.first;
      if (e.second > 1) ss << e.second;
    }
  }
  return ss.str();
}

MolecularGraph IXMolecule::GetMolecularGraph() { return graph_; }

std::string IXMolecule::GetName() const { return name_; }

/** @details Issues a warning if the set molecular charge does not match the
 *  sum of the atom formal charges.
 */
int IXMolecule::GetTotalCharge() const {
  int q_tot = 0;
  for (Atom atm : atoms_) q_tot += atm->GetFormalCharge();
  /// @todo Logging system call
  if (q_tot != q_) std::cerr << "Atom formal charges do not match molecular charge." << std::endl;
  return q_;
}

bool IXMolecule::IsModified() const { return modified_; }

size_t IXMolecule::NumAtoms() const { return (size_t)atoms_.size(); }

size_t IXMolecule::NumBonds() const { return (size_t)bonds_.size(); }

/// @name Data modification methods

/** @param name the name tos et for this molecule. */
void IXMolecule::SetName(std::string name) { name_ = name; }

/** @param q the molecular charge value to set. */
void IXMolecule::SetTotalCharge(int q) {
  q_ = q;
  modified_ = true;
  graph_->SetTotalCharge(q);
}

void IXMolecule::ResetIndices() {
  idx_to_atom_.clear();
  for (uid_t i = 0; i < atoms_.size(); ++i) {
    atoms_[i]->SetIndex(i);
    idx_to_atom_.emplace(i, atoms_[i]);
  }
  
  idx_to_bond_.clear();
  for (uid_t i = 0; i < bonds_.size(); ++i) {
    bonds_[i]->SetIndex(i);
    idx_to_bond_.emplace(i, bonds_[i]);
  }
}

Atom IXMolecule::NewAtom() {
  Atom atm = Atom(new IXAtom(shared_from_this()));
  atoms_.emplace_back(atm);
  idx_to_atom_.emplace(atm->GetIndex(), atm);
  atom_to_vertex_.emplace(atm, graph_->AddVertex(atm));
  modified_ = true;
  return atm;
}


Atom IXMolecule::NewAtom(Element e) {
  Atom atm = NewAtom();
  atm->SetElement(e);
  return atm;
}
Atom IXMolecule::NewAtom(uid_t idx, Element e) {
  Atom atm = NewAtom(e);
  idx_to_atom_.erase(atm->GetIndex());
  atm->SetIndex(idx);
  idx_to_atom_.emplace(idx, atm);
  return atm;
}

/** @param a the source atom for the new bond.
 *  @param b the target atom for the new bond.
 *  @details Both atoms should be in this molecule.
 */
Bond IXMolecule::NewBond(Atom a, Atom b) {
  /// @todo Logging for fail reasons.
  Bond bnd = Bond();
  if (a->GetMolecule() != shared_from_this()) return bnd;
  if (b->GetMolecule() != shared_from_this()) return bnd;
  bnd = GetBond(a, b);
  if (!bnd) {
    bnd.reset(new IXBond(a, b));
    bonds_.emplace_back(bnd);
    idx_to_bond_.emplace(bnd->GetIndex(), bnd);
    MolEdgeBool eb = graph_->AddEdge(atom_to_vertex_.at(a),
                                     atom_to_vertex_.at(b), bnd);
    bond_to_edge_.emplace(bnd, eb.first);
    a->AddBond(bnd);
    b->AddBond(bnd);
    modified_ = true;
  } // else Log that bond already exists.
  return bnd;
}

/** @param a the atom to remove from the molecule. */
void IXMolecule::RemoveAtom(Atom a) {
  if (a->GetMolecule() != shared_from_this()) return;
  auto it = std::find(atoms_.begin(), atoms_.end(), a);
  if (it != atoms_.end()) {
    Atom hit = *it;
    for (auto bs = hit->GetBondIters(); bs.first != bs.second; ++bs.first) {
      Bond tmp = bs.first->lock();
      bond_to_edge_.erase(tmp);
      if (tmp->GetSourceAtom() != hit) tmp->GetSourceAtom()->RemoveBond(tmp);
      else tmp->GetTargetAtom()->RemoveBond(tmp);
      tmp->Clear();
    }
    atoms_.erase(it);
    graph_->RemoveVertex(atom_to_vertex_.at(hit));
    atom_to_vertex_.erase(hit);
    hit->Clear();
    modified_ = true;
  }
}

/** @param b the bond to remove from the molecule. */
void IXMolecule::RemoveBond(Bond b) {
  if (b->GetMolecule() != shared_from_this()) return;
  auto it = std::find(bonds_.begin(), bonds_.end(), b);
  if (it != bonds_.end()) {
    Bond hit = *it;
    hit->GetSourceAtom()->RemoveBond(hit);
    hit->GetTargetAtom()->RemoveBond(hit);
    bonds_.erase(it);
    bond_to_edge_.erase(hit);
    hit->Clear();
    modified_ = true;
  }
}

size_t IXMolecule::AssignElectrons() {
  // check is single component (use graph)
  /// @todo
  if (graph_->NumConnectedComponents() != 1) {
    std::cerr << "Electrons can only be assigned when molecule has only one component." << std::endl;
    return 0;
  }
  
  // check all elements valid.
  typedef Options::AssignElectrons opt_;
  for (Atom atm : atoms_) {
    if (!atm->GetElement()) {
      std::cerr << "Not all atoms have elements assigned." << std::endl;
      return 0;
    }
    if (opt_::ALLOWED_ELEMENTS.find(atm->GetElement()->GetSymbol())
        == opt_::ALLOWED_ELEMENTS.end()) {
      std::cerr << atm->GetElement() << " is not an allowed element for the algorithm." << std::endl;
      return 0;
    }
  }
  
  // Set the ElectronOpt graph
  elnopt_->SetMolecularGraph(graph_);
  modified_ = false;
  size_t num = (size_t)elnopt_->Run();
  if (num) ApplyElectronAssignment(0);
//  for (auto& tmp : idx_to_atom_) {
//    Atom_p atom = tmp.second.lock();
//    if (atom->GetFormalCharge() != 0) {
//      std::cout << tmp.first << " : " << atom->GetFormalCharge() << std::endl;
//    }
//  }
//  
//  for (auto& tmp : idx_to_bond_) {
//    Bond_p bond = tmp.second.lock();
//    if (bond->GetOrder() != BondOrder::SINGLE_BOND) {
//      uid_t a = bond->GetSourceAtom()->GetIndex();
//      uid_t b = bond->GetTargetAtom()->GetIndex();
//      if (a <= b) std::cout << a << "-" << b << " : " << bond->GetOrder() << std::endl;
//      else std::cout << b << "-" << a << " : " << bond->GetOrder() << std::endl;
//    }
//  }
  return num;
}

FCSCORE IXMolecule::GetMinimumElectronAssignmentScore() {
  if (modified_) {
    std::cerr << "Molecule has been modified since assignment. Please re-assign." << std::endl;
    return Options::AssignElectrons::INF;
  }
  return elnopt_->GetMinimisedEnergy();
}

bool IXMolecule::ApplyElectronAssignment(size_t idx) {
  if (modified_) {
    std::cerr << "Molecule has been modified since assignment. Please re-assign." << std::endl;
    return false;
  }
  return elnopt_->ApplyElectronAssigment(idx);
}

// Iterator methods
Atom IXMolecule::Begin(MolAtomIterator &it) {
  it = atoms_.begin();
  return (it == atoms_.end()) ? Atom() : *it;
}

Atom IXMolecule::Next(MolAtomIterator &it) {
  ++it;
  return (it == atoms_.end()) ? Atom() : *it;
}

MolAtomIterator IXMolecule::BeginAtom() { return atoms_.begin(); }
MolAtomIterator IXMolecule::EndAtom() { return atoms_.end(); }
MolBondIterator IXMolecule::BeginBond() { return bonds_.begin(); }
MolBondIterator IXMolecule::EndBond() { return bonds_.end(); }


