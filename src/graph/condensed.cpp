#include <indigox/classes/atom.hpp>
#include <indigox/classes/bond.hpp>
#include <indigox/classes/periodictable.hpp>
#include <indigox/graph/condensed.hpp>
#include <indigox/graph/molecular.hpp>
#include <indigox/utils/serialise.hpp>

#include <EASTL/vector_set.h>
#include <algorithm>

namespace indigox::graph {

  eastl::vector_set<Element> __con_elem = {
      GetPeriodicTable().GetElement("H"), GetPeriodicTable().GetElement("F"),
      GetPeriodicTable().GetElement("Cl"), GetPeriodicTable().GetElement("Br"),
      GetPeriodicTable().GetElement("I")};

  struct CMGVertex::CMGVertexData {
    MGVertex source;
    CondensedMolecularGraph graph;
    std::vector<CondensedVertex> condensed;
    VertexIsoMask mask;

    CMGVertexData() = default;
    CMGVertexData(const MGVertex &v, const CondensedMolecularGraph &g)
        : source(v), graph(g), mask(0) {}

    template <typename Archive>
    void serialise(Archive &archive, const uint32_t) {
      archive(INDIGOX_SERIAL_NVP("source", source),
              INDIGOX_SERIAL_NVP("graph", graph),
              INDIGOX_SERIAL_NVP("condensed_verts", condensed),
              INDIGOX_SERIAL_NVP("mask", mask));
    }
  };

  struct CMGEdge::CMGEdgeData {
    MGEdge source;
    CondensedMolecularGraph graph;
    EdgeIsoMask mask;

    CMGEdgeData() = default;
    CMGEdgeData(const MGEdge &e, const CondensedMolecularGraph &g)
        : source(e), graph(g), mask(0) {}

    template <typename Archive>
    void serialise(Archive &archive, const uint32_t) {
      archive(INDIGOX_SERIAL_NVP("source", source),
              INDIGOX_SERIAL_NVP("graph", graph),
              INDIGOX_SERIAL_NVP("mask", mask));
    }
  };

  struct CondensedMolecularGraph::Impl {
    VertMap condensed_vertices;
    EdgeMap condensed_edges;
    MolecularGraph graph;
    CondensedMolecularGraph super_graph;

    Impl() = default;
    Impl(const MolecularGraph &g) : graph(g) {}

    template <typename Archive>
    void serialise(Archive &archive, const uint32_t) {
      archive(INDIGOX_SERIAL_NVP("vertices", condensed_vertices),
              INDIGOX_SERIAL_NVP("edges", condensed_edges),
              INDIGOX_SERIAL_NVP("molecular_graph", graph),
              INDIGOX_SERIAL_NVP("supergraph", super_graph));
    }
  };

  // ==================================================================
  // == CMGVertex CONSTRUCTION ========================================
  // ==================================================================

  CMGVertex::ContractedSymmetry __get_symmetry(size_t atomic_number,
                                               const MGVertex &) {
    using CS = CMGVertex::ContractedSymmetry;
    switch (atomic_number) {
    case 1: return CS::Hydrogen;
    case 9: return CS::Fluorine;
    case 17: return CS::Chlorine;
    case 35: return CS::Bromine;
    case 53: return CS::Iodine;
    default: throw std::runtime_error("Cannot determine contracted symmetry");
    }
  }

  CMGVertex::CMGVertex(const MGVertex &v, const CondensedMolecularGraph &g)
      : m_data(std::make_shared<CMGVertexData>(v, g)) {
    using CS = ContractedSymmetry;
    MolecularGraph MG = g.GetMolecularGraph();
    const MolecularGraph::VertContain &nbrs = MG.GetNeighbours(v);
    // Condensed vertices for the non-leaf addings
    if (nbrs.size() > 1) {
      for (const MGVertex &u : nbrs) {
        MGEdge e = MG.GetEdge(u, v);
        Atom atm = u.GetAtom();
        Bond bnd = e.GetBond();
        if ((__con_elem.find(atm.GetElement()) != __con_elem.end()) &&
            (atm.GetFormalCharge() == 0) &&
            (bnd.GetOrder() == BondOrder::SINGLE)) {
          size_t el = atm.GetElement().GetAtomicNumber();
          CondensedVertex u_con = std::make_pair(__get_symmetry(el, u), u);
          m_data->condensed.emplace_back(u_con);
        }
      }
      std::sort(m_data->condensed.begin(), m_data->condensed.end());
    }

    // Determine Isomorphism Mask
    // Element number (7 bits)
    // Magnitude of formal charge (3 bits)
    // Is FC negative (1 bit)
    // Num condensed H (3 bits)
    // Num condensed F (3 bits)
    // Num condensed Cl (3 bits)
    // Num condensed Br (3 bits)
    // Num condensed I (3 bits)
    // Is in any cycle (1 bit)          // removed
    // Is in small cycle (<= 8)(1 bit)
    // Is R stereo (1 bit)
    // Is S stereo (1 bit)
    // Is aromatic (1 bit)             // removed
    // Num implicit H (3 bits)
    Atom atm = v.GetAtom();
    VertexIsoMask atm_num, fc_mag, h, f, cl, br, i, mask, degree, imp_h;
    atm_num.from_uint64(atm.GetElement().GetAtomicNumber());
    fc_mag.from_uint64(abs(atm.GetFormalCharge()));
    fc_mag <<= 7;
    h.from_uint32(NumContracted(CS::Hydrogen));
    h <<= 11;
    f.from_uint32(NumContracted(CS::Fluorine));
    f <<= 14;
    cl.from_uint32(NumContracted(CS::Chlorine));
    cl <<= 17;
    br.from_uint32(NumContracted(CS::Bromine));
    br <<= 20;
    i.from_uint32(NumContracted(CS::Iodine));
    i <<= 23;
    degree.from_uint32(atm.NumBonds());
    degree <<= 31;
    imp_h.from_uint32(atm.GetImplicitCount());
    imp_h <<= 34;

    mask = atm_num | fc_mag | h | f | cl | br | i | degree | imp_h;
    if (atm.GetFormalCharge() < 0) mask.set(10);
    if (MG.IsCyclic(v, 8)) mask.set(26);
    if (atm.GetStereochemistry() == AtomStereo::R) mask.set(28);
    if (atm.GetStereochemistry() == AtomStereo::S) mask.set(29);

    m_data->mask = mask;
  }

  // ====================================================================
  // == CMGVertex Serialisation =========================================
  // ====================================================================
  template <typename Archive>
  void CMGVertex::serialise(Archive &archive, const uint32_t) {
    archive(INDIGOX_SERIAL_NVP("data", m_data));
  }
  INDIGOX_SERIALISE(CMGVertex);

  // ====================================================================
  // == CMGVertex Methods ===============================================
  // ====================================================================
  const MGVertex &CMGVertex::GetSource() const { return m_data->source; }

  const CondensedMolecularGraph &CMGVertex::GetGraph() const {
    return m_data->graph;
  }

  size_t CMGVertex::NumContracted() const { return m_data->condensed.size(); }

  size_t CMGVertex::NumContracted(ContractedSymmetry s) const {
    return std::accumulate(m_data->condensed.begin(), m_data->condensed.end(),
                           0,
                           [&s](size_t a, const CondensedVertex &v) -> size_t {
                             return s == v.first ? ++a : a;
                           });
  }

  const VertexIsoMask &CMGVertex::GetIsomorphismMask() const {
    return m_data->mask;
  }

  bool CMGVertex::IsContractedHere(const MGVertex &v) const {
    size_t z = v.GetAtom().GetElement().GetAtomicNumber();
    CondensedVertex cv = std::make_pair(__get_symmetry(z, v), v);
    return std::find(m_data->condensed.begin(), m_data->condensed.end(), cv) !=
           m_data->condensed.end();
  }

  std::vector<MGVertex> CMGVertex::GetContractedVertices() const {
    std::vector<MGVertex> verts;
    verts.reserve(NumContracted());
    for (auto &cv : m_data->condensed) verts.emplace_back(cv.second);
    return verts;
  }

  const std::vector<CMGVertex::CondensedVertex> &
  CMGVertex::GetCondensedVertices() const {
    return m_data->condensed;
  }

  // =======================================================================
  // == CMGEdge CONSTRUCTION ===============================================
  // =======================================================================

  CMGEdge::CMGEdge(const MGEdge &e, const CondensedMolecularGraph &g)
      : m_data(std::make_shared<CMGEdgeData>(e, g)) {
    EdgeIsoMask mask, degree_small, degree_large;
    MolecularGraph G = g.GetMolecularGraph();
    // Determine Isomorphism Mask
    // Order (3 bits)
    // Is E stereo (1 bit)
    // Is Z stereo (1 bit)
    // Is in any cycle (1 bit)
    // Is in small cycle (<= 8)(1 bit)
    // Is aromatic (1 bit)
    Bond bnd = e.GetBond();
    Atom a = bnd.GetAtoms()[0];
    Atom b = bnd.GetAtoms()[1];
    if (a.NumBonds() > b.NumBonds()) {
      degree_small.from_uint32(b.NumBonds());
      degree_large.from_uint32(a.NumBonds());
    } else {
      degree_small.from_uint32(a.NumBonds());
      degree_large.from_uint32(b.NumBonds());
    }
    degree_small <<= 8;
    degree_large <<= 11;
    mask.from_uint32(static_cast<uint32_t>(bnd.GetOrder()));
    mask |= degree_small | degree_large;
    if (bnd.GetStereochemistry() == BondStereo::E) mask.set(3);
    if (bnd.GetStereochemistry() == BondStereo::Z) mask.set(4);
    if (G.IsCyclic(e, 8)) mask.set(5);
    //    if (e->IsCyclic(8)) _iso_mask.set(6);
    //    if (bnd.GetAromaticity())
    //      mask.set(7);
    m_data->mask = mask;
  }

  // ======================================================================
  // == CMGEdge Serialisation =============================================
  // ======================================================================
  template <typename Archive>
  void CMGEdge::serialise(Archive &archive, const uint32_t) {
    archive(INDIGOX_SERIAL_NVP("data", m_data));
  }
  INDIGOX_SERIALISE(CMGEdge);

  // ======================================================================
  // == CMGEdge Methods ===================================================
  // ======================================================================

  const MGEdge &CMGEdge::GetSource() const { return m_data->source; }
  const CondensedMolecularGraph &CMGEdge::GetGraph() const {
    return m_data->graph;
  }
  const EdgeIsoMask &CMGEdge::GetIsomorphismMask() const {
    return m_data->mask;
  }

  // ======================================================================
  // == CondensedMolecularGraph Serialisation =============================
  // ======================================================================

  template <typename Archive>
  void CondensedMolecularGraph::serialise(Archive &archive, const uint32_t) {
    archive(
        INDIGOX_SERIAL_NVP("base_graph", cereal::base_class<graph_type>(this)),
        INDIGOX_SERIAL_NVP("data", m_data));
  }
  INDIGOX_SERIALISE(CondensedMolecularGraph);

  // =======================================================================
  // == CondensedMolecularGraph Modification ===============================
  // =======================================================================

  CMGEdge CondensedMolecularGraph::AddEdge(const MGEdge &e) {
    MolecularGraph MG = e.GetGraph();
    CMGVertex u = GetVertex(MG.GetSourceVertex(e));
    CMGVertex v = GetVertex(MG.GetTargetVertex(e));
    CMGEdge e_local = CMGEdge(e, *this);
    m_data->condensed_edges.emplace(e, e_local);
    graph_type::AddEdge(u, v, e_local);
    return e_local;
  }

  CMGVertex CondensedMolecularGraph::AddVertex(const MGVertex &v) {
    CMGVertex v_locl = CMGVertex(v, *this);
    m_data->condensed_vertices.emplace(v, v_locl);
    graph_type::AddVertex(v_locl);
    return v_locl;
  }

  // =======================================================================
  // == CondensedMolecularGraph CONSTRUCTION ===============================
  // =======================================================================

  CondensedMolecularGraph::CondensedMolecularGraph(const MolecularGraph &g)
      : m_data(std::make_shared<Impl>(g)) {}

  CondensedMolecularGraph Condense(const MolecularGraph &MG) {
    using CMG = CondensedMolecularGraph;
    MolecularGraph mg = MG;
    CMG CG(MG);

    for (const MGVertex &v : MG.GetVertices()) {
      // Add vertex for all non-leaf vertices
      if (MG.Degree(v) > 1) {
        CG.AddVertex(v);
        continue;
      }
      const Atom& atm = v.GetAtom();
      const Element& e = atm.GetElement();
      // Add vertex if leaf vertex is not in __con_elem
      if (__con_elem.find(e) == __con_elem.end()) {
        CG.AddVertex(v);
        continue;
      }
      // Add vertex if has formal charge
      if (atm.GetFormalCharge() != 0) {
        CG.AddVertex(v);
        continue;
      }
      // Add vertex if bond is not single
      MGVertex u = mg.GetNeighbours(v).front();
      BondOrder order = MG.GetEdge(v, u).GetBond().GetOrder();
      if (order != BondOrder::SINGLE) {
        CG.AddVertex(v);
        continue;
      }
      // Add vertex if parent is also a leaf
      if (MG.Degree(u) == 1) CG.AddVertex(v);
    }

    for (const MGEdge &e : MG.GetEdges()) {
      MGVertex u, v;
      std::tie(u, v) = MG.GetVertices(e);
      if (CG.HasVertex(u) && CG.HasVertex(v)) CG.AddEdge(e);
    }
    return CG;
  }

  // ======================================================================
  // == CondensedMolecularGraph Getting and Checking ======================
  // ======================================================================
  const MolecularGraph &CondensedMolecularGraph::GetMolecularGraph() const {
    if (IsSubgraph())
      throw std::runtime_error("Subgraphs do not relate to molecular graphs");
    return m_data->graph;
  }

  const CondensedMolecularGraph &
  CondensedMolecularGraph::GetSuperGraph() const {
    if (!IsSubgraph())
      throw std::runtime_error("Cannot get supergraph of non subgraph");
    return m_data->super_graph;
  }

  const CMGEdge &CondensedMolecularGraph::GetEdge(const MGEdge &e) const {
    return m_data->condensed_edges.at(e);
  }

  const CMGVertex &CondensedMolecularGraph::GetVertex(const MGVertex &v) const {
    return m_data->condensed_vertices.at(v);
  }

  const CMGVertex &
  CondensedMolecularGraph::GetCondensedVertex(const MGVertex &v) const {
    return *std::find_if(
        m_basedata->vertices.begin(), m_basedata->vertices.end(),
        [&v](const CMGVertex &u) { return u.IsContractedHere(v); });
  }

  bool CondensedMolecularGraph::HasVertex(const MGVertex &v) const {
    return m_data->condensed_vertices.find(v) !=
           m_data->condensed_vertices.end();
  }

  bool CondensedMolecularGraph::HasEdge(const MGEdge &e) const {
    return m_data->condensed_edges.find(e) != m_data->condensed_edges.end();
  }

  bool CondensedMolecularGraph::HasCondensedVertex(const MGVertex &v) const {
    return std::find_if(m_basedata->vertices.begin(),
                        m_basedata->vertices.end(), [&v](const CMGVertex &u) {
                          return u.IsContractedHere(v);
                        }) != m_basedata->vertices.end();
  }

  bool CondensedMolecularGraph::IsSubgraph() const {
    return bool(m_data->super_graph);
  }

  // =======================================================================
  // == CondensedMolecularGraph Subgraph generation ========================
  // =======================================================================
  CondensedMolecularGraph
  CondensedMolecularGraph::Subgraph(std::vector<CMGVertex> &verts) {
    CondensedMolecularGraph G;
    G.m_data = std::make_shared<Impl>();
    G.m_data->super_graph = *this;

    for (const CMGVertex &v : verts) {
      if (!HasVertex(v)) throw std::runtime_error("Non-member vertex");
      MGVertex source = v.GetSource();
      G.m_data->condensed_vertices.emplace(source, v);
      G.graph_type::AddVertex(v);
    }

    for (const CMGEdge &e : GetEdges()) {
      CMGVertex u = GetSourceVertex(e);
      CMGVertex v = GetTargetVertex(e);
      if (!G.HasVertex(u) || !G.HasVertex(v)) continue;
      G.m_data->condensed_edges.emplace(e.GetSource(), e);
      G.graph_type::AddEdge(u, v, e);
    }
    return G;
  }

  CondensedMolecularGraph
  CondensedMolecularGraph::Subgraph(std::vector<CMGVertex> &verts,
                                    std::vector<CMGEdge> &edges) {
    CondensedMolecularGraph G;
    G.m_data = std::make_shared<Impl>();
    G.m_data->super_graph = *this;

    for (const CMGVertex &v : verts) {
      if (!HasVertex(v)) throw std::runtime_error("Non-member vertex");
      MGVertex source = v.GetSource();
      G.m_data->condensed_vertices.emplace(source, v);
      G.graph_type::AddVertex(v);
    }

    for (const CMGEdge &e : edges) {
      if (!HasEdge(e)) throw std::runtime_error("Non-member edge");
      CMGVertex u = GetSourceVertex(e);
      CMGVertex v = GetTargetVertex(e);
      if (!G.HasVertex(u) || !G.HasVertex(v)) continue;
      G.m_data->condensed_edges.emplace(e.GetSource(), e);
      G.graph_type::AddEdge(u, v, e);
    }
    return G;
  }

  // =======================================================================
  // == Operators ==========================================================
  // =======================================================================

  bool CMGVertex::operator==(const CMGVertex &v) const {
    return m_data->source == v.m_data->source;
  }

  bool CMGVertex::operator<(const CMGVertex &v) const {
    return m_data->source < v.m_data->source;
  }

  bool CMGVertex::operator>(const CMGVertex &v) const {
    return m_data->source > v.m_data->source;
  }

  std::ostream &operator<<(std::ostream &os, const CMGVertex &v) {
    if (v) {
      os << "CMGVertex(" << v.GetSource().GetAtom().GetIndex() + 1 << ")";
    }
    return os;
  }

  bool CMGEdge::operator==(const CMGEdge &v) const {
    return m_data->source == v.m_data->source;
  }

  bool CMGEdge::operator<(const CMGEdge &v) const {
    return m_data->source < v.m_data->source;
  }

  bool CMGEdge::operator>(const CMGEdge &v) const {
    return m_data->source > v.m_data->source;
  }

  std::ostream &operator<<(std::ostream &os, const CMGEdge &e) {
    if (e) {
      os << "CMGEdge(" << e.GetSource().GetBond().GetIndex() + 1 << ")";
    }
    return os;
  }

  bool CondensedMolecularGraph::
  operator==(const CondensedMolecularGraph &g) const {
    return m_data == g.m_data;
  }

  bool CondensedMolecularGraph::
  operator<(const CondensedMolecularGraph &g) const {
    return m_data < g.m_data;
  }

  bool CondensedMolecularGraph::
  operator>(const CondensedMolecularGraph &g) const {
    return m_data > g.m_data;
  }

  std::ostream &operator<<(std::ostream &os, const CondensedMolecularGraph &G) {
    if (G) {
      os << "CondensedMolecularGraph(" << G.NumVertices() << " vertices, "
         << G.NumEdges() << " edges)";
    }
    return os;
  }

} // namespace indigox::graph
