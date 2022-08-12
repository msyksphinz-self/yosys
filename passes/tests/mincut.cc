#include "kernel/yosys.h"
#include "kernel/satgen.h"
#include "kernel/consteval.h"
#include "kernel/celledges.h"
#include "kernel/macc.h"
#include <algorithm>
#include <vector>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct Mincut : public Pass {

  typedef unsigned int hash_t;

  std::vector<hash_t> m_vec_cell_hash;

  Mincut() : Pass("mincut", "Minimum Cut") { }
  void help() override
  {
    //    |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log ("\n");
    log ("    mincut\n");
    log ("\n");
  }

  void execute(std::vector<std::string> args, RTLIL::Design* design) override
  {
    log ("Mincut %lu!\n", args.size());
    auto modules = design->modules();
    for (auto iter = modules.begin(); iter != modules.end(); iter++) {
      log("Module Name = %s\n", (*iter)->name.c_str());
      int num_cells = (*iter)->cells().size();
      log("Cells = %d\n", num_cells);

      for (auto cell : (*iter)->cells()) {
        m_vec_cell_hash.push_back (cell->hash());
      }

      std::vector<std::vector<hash_t>> vec_cell_table(num_cells, std::vector<hash_t>(num_cells, 0));

      // Dump
      for (auto cell : (*iter)->cells()) {
        log("%x\n", cell->hashidx_);
      }

      // Connections
      for (auto cell : (*iter)->cells()) {
        for (auto conn : cell->connections()) {
          // log("%s =  %x\n", conn.first.c_str(),
          //     get_hash_idx (conn.second));

          std::vector<hash_t> *target_cells = search_target_cell((*iter), cell->hash(), get_hash_idx (conn.second));
          for (auto target : *target_cells) {
            hash_t cell_idx = get_idx_by_hash(cell->hash());
            hash_t target_idx = get_idx_by_hash(target);
            log ("cell_hash(%d)=%d\ntarget(%d)=%d\n", cell->hash(), cell_idx, target, target_idx);
            vec_cell_table[cell_idx][target_idx] = 1;
          }
        }
      }

      // Dump
      for (hash_t y = 0; y < vec_cell_table.size(); y++) {
        for(hash_t x = 0; x < vec_cell_table.size(); x++) {
          log ("%d ", vec_cell_table[y][x]);
        }
        log ("\n");
      }

      class mincut_node_t {
        hash_t hash;
        int diff;
      };

      std::vector<hash_t> vec_grp_a;
      std::vector<hash_t> vec_grp_b;



      for (int i = 0; i < num_cells / 2; i++) {
        vec_grp_a.push_back (m_vec_cell_hash[i]);
      }
      for (int i = num_cells / 2; i < num_cells; i++) {
        vec_grp_b.push_back (m_vec_cell_hash[i]);
      }
    }
  }

  int get_idx_by_hash (hash_t hash) {
    for (hash_t i = 0; i < m_vec_cell_hash.size(); i++) {
      if (m_vec_cell_hash[i] == hash) {
        return i;
      }
    }
    log ("Can not find hash number %x\n", hash);
    exit (1);
  }

  std::vector<hash_t> * search_target_cell (RTLIL::Module *module, hash_t from_cell_idx, hash_t wire_cell_idx) {
    std::vector<hash_t> *ret_vec = new std::vector<hash_t> ();

    for (auto cell : module->cells()) {
      if (cell->hash() == from_cell_idx) {
        continue;
      }
      for (auto conn : cell->connections()) {
        if (get_hash_idx (conn.second) == wire_cell_idx) {
          ret_vec->push_back(cell->hash());
        }
      }
    }

    return ret_vec;
  }

  hash_t get_hash_idx (RTLIL::SigSpec sigspec) {
    if (sigspec.is_chunk()) {
      return get_hash_idx(sigspec.as_chunk());
    } else if (sigspec.is_wire()) {
      return get_hash_idx(sigspec.as_wire());
    } else {
      log ("Unexpected search_sigspec\n");
      exit (1);
    }
  }

  hash_t get_hash_idx (RTLIL::Wire *wire) {
    return wire->hash();
  }

  hash_t get_hash_idx (RTLIL::SigChunk chunk) {
    if (chunk.wire) {
      return get_hash_idx (chunk.wire);
    } else {
      return -1;
    }
  }


} Mincut;

PRIVATE_NAMESPACE_END
