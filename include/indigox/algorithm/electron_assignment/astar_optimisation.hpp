#ifndef INDIGOX_ALGORITHM_ELECTRON_ASSIGNMENT_ASTAR_OPTIMISATION_HPP
#define INDIGOX_ALGORITHM_ELECTRON_ASSIGNMENT_ASTAR_OPTIMISATION_HPP

#include "../../graph/assignment.hpp"
#include "assigner.hpp"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#include <algorithm>
#include <cstdint>
#include <queue>

namespace indigox::algorithm {

  using LocMask = boost::dynamic_bitset<>;

  struct QueueItem {
    score_t path, heuristic;
    AssignMask assignment;
    LocMask unchange_mask, calc_mask, new_calc_mask;
    size_t nbr_begin_idx;

    inline bool IsInfinite() const {
      static const score_t inf = IXElectronAssigner::Settings::Infinity;
      return path == inf || heuristic == inf;
    }
    inline score_t Total() const {
      return path + heuristic;
    }
    inline size_t CalcCount() const {
      return calc_mask.count();
    }
    bool operator>(const QueueItem &r) const;
    QueueItem() = default;
    QueueItem(const size_t loc_size, const size_t pos_size)
        : path(0), heuristic(0), assignment(loc_size), unchange_mask(pos_size),
          calc_mask(pos_size), new_calc_mask(pos_size), nbr_begin_idx(0) {
    }

    QueueItem(const score_t p, const score_t h, const AssignMask &ass,
              const LocMask &unchange, const LocMask &calc,
              const LocMask &new_calc, const size_t ni)
        : path(p), heuristic(h), assignment(ass), unchange_mask(unchange),
          calc_mask(calc), new_calc_mask(new_calc), nbr_begin_idx(ni) {
    }
  };

  using queue_t = std::priority_queue<QueueItem, std::vector<QueueItem>,
                                      std::greater<QueueItem>>;
  class PriorityQueue : public queue_t {
  public:
    void reserve(size_t sz) {
      this->c.reserve(sz);
    }
    size_t max_size() const {
      return this->c.max_size();
    }
    void clear() {
      this->c.clear();
    }
  };

  class IXAStarOptimisation : public IXElectronAssigner::AssignAlgorithm {
    using NbrAssigns = std::array<AssignMask, 32>;

  public:
    enum class Heuristic { Promiscuous, Abstemious };

    struct Settings {
      static uint64_t MEMORY_LIMIT;
      static Heuristic HEURISTIC;
    };

  public:
    IXAStarOptimisation() = delete; // no default constructor

    /*! \brief Normal constructor.
     *  \details Sets the options of the algorithm from their state at the time
     *  of construction. Also calls the base constructor.
     *  \param t the score table to use. */
    IXAStarOptimisation(const ScoreTable &t);

    /*! \brief Initalisation method.
     *  \details Overrides the base class initalisation method so that location
     *  masks can be generated as part of initalisation.
     *  \param mol the Molecule to initalise with. */
    virtual void Initalise(const Molecule &mol) override;

    /*! \brief Run the algorithm.
     *  \details Runs the algorithm and populates the results. */
    virtual void Run() override;

  private:
    void DetermineLocationCounts();
    void DetermineRequiredUnchangeables();
    void DetermineInitialAssignment();
    LocMask DetermineCalculableLocations(const QueueItem &q) const;
    score_t CalculateNewPathScore(const QueueItem &q);
    score_t CalculateHeuristicScore(const QueueItem &q) const;
    score_t Promiscuous(const QueueItem &q) const;
    score_t Abstemious(const QueueItem &q) const;
    size_t GenerateNextAssignments(const QueueItem &q, NbrAssigns &out) const;

    inline size_t GetLocationPosition(const graph::AGVertex &v) const {
      return std::distance(
          _unique_locs.begin(),
          std::find(_unique_locs.begin(), _unique_locs.end(), v));
    }

  private:
    PriorityQueue _q;
    uint64_t _len_limit;
    std::vector<graph::AGVertex> _unique_locs;
    std::vector<uint32_t> _loc_counts;
    std::vector<LocMask> _req_unchange;
  };
} // namespace indigox::algorithm

#endif /* INDIGOX_ALGORITHM_ELECTRON_ASSIGNMENT_ASTAR_OPTIMISATION_HPP */
