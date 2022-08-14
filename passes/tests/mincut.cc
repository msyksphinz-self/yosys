#include "kernel/yosys.h"
#include "kernel/satgen.h"
#include "kernel/consteval.h"
#include "kernel/celledges.h"
#include "kernel/macc.h"

#include <assert.h>
#include <algorithm>
#include <vector>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct Mincut : public Pass {

  typedef unsigned int hash_t;

  // int get_idx_by_hash (hash_t hash);
  // std::vector<hash_t> * search_target_cell (RTLIL::Module *module, hash_t from_cell_idx, hash_t wire_cell_idx);
  // hash_t get_hash (RTLIL::SigSpec sigspec);
  // hash_t get_hash (RTLIL::Wire *wire);
  // hash_t get_hash (RTLIL::SigChunk chunk);

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

    std::vector<hash_t> vec_grp_a;
    std::vector<hash_t> vec_grp_b;


    for (auto iter = modules.begin(); iter != modules.end(); iter++) {
      log("Module Name = %s\n", (*iter)->name.c_str());
      size_t init_num_cells = (*iter)->cells().size();
      log("Cells = %zu\n", init_num_cells);

      for (auto cell : (*iter)->cells()) {
        m_vec_cell_hash.push_back (cell->hash());
      }

      std::vector<std::vector<hash_t>> vec_cell_table(init_num_cells, std::vector<hash_t>(init_num_cells, 0));

      // Dump
      for (auto cell : (*iter)->cells()) {
        log("%x ", cell->hashidx_);
      }
      log ("\n");

      // Connections
      for (auto cell : (*iter)->cells()) {
        for (auto conn : cell->connections()) {
          // log("%s =  %x\n", conn.first.c_str(),
          //     get_hash (conn.second));

          std::vector<hash_t> *target_cells = search_target_cell((*iter), cell->hash(), get_hash (conn.second));
          for (auto target : *target_cells) {
            hash_t cell_idx   = get_idx_by_hash(cell->hash());
            hash_t target_idx = get_idx_by_hash(target);
            // log ("cell_hash_idx(%08x)=%d, target_idx(%08x)=%d\n", cell->hash(), cell_idx, target, target_idx);
            vec_cell_table[cell_idx][target_idx] = 1;
          }
        }
      }

      // Dump
      // for (hash_t y = 0; y < vec_cell_table.size(); y++) {
      //   log ("%10d : ", y);
      //   for(hash_t x = 0; x < vec_cell_table.size(); x++) {
      //     log ("%d ", vec_cell_table[y][x]);
      //   }
      //   log ("\n");
      // }


      for (size_t i = 0; i < init_num_cells / 2; i++) {
        vec_grp_a.push_back (m_vec_cell_hash[i]);
      }
      for (size_t i = init_num_cells / 2; i < init_num_cells; i++) {
        vec_grp_b.push_back (m_vec_cell_hash[i]);
      }

      int tmp_cnt = 0;
      do {

        // log ("Mincut Do While Loop, tmp_cnt = %d\n", tmp_cnt);

        size_t num_cells = vec_grp_a.size() + vec_grp_b.size();

        std::vector<int> gv;
        std::vector<hash_t> av;
        std::vector<hash_t> bv;

        // Mincut iteration loop
        for (size_t loop = 0; loop < num_cells / 2; loop++) {

          // log ("Mincut Iteration Loop : %zu, Num Cells = %zu, tmp_cnt = %d\n", loop, num_cells, tmp_cnt);

          std::vector<int> cell_diff_table;
          // log ("cell_diff_table : ");
          for (auto a0 : vec_grp_a) {
            int d = 0;
            size_t a0_idx = get_idx_by_hash (a0);
            for (auto a1 : vec_grp_a) {
              size_t a1_idx = get_idx_by_hash (a1);
              d += vec_cell_table[a0_idx][a1_idx];
            }
            for (auto b1 : vec_grp_b) {
              size_t b1_idx = get_idx_by_hash (b1);
              d -= vec_cell_table[a0_idx][b1_idx];
            }
            cell_diff_table.push_back(d);
            // log ("%3d ", d);
          }

          // log (" | ");
          for (auto b0 : vec_grp_b) {
            int d = 0;
            size_t b0_idx = get_idx_by_hash (b0);
            for (auto b1 : vec_grp_b) {
              size_t b1_idx = get_idx_by_hash (b1);
              d += vec_cell_table[b0_idx][b1_idx];
            }
            for (auto a1 : vec_grp_a) {
              size_t a1_idx = get_idx_by_hash (a1);
              d -= vec_cell_table[b0_idx][a1_idx];
            }
            cell_diff_table.push_back(d);
            // log ("%3d ", d);
          }
          // log ("\n");

          // log ("g = DA + DB - 2AB tables :\n");
          // find a from A and b from B, such that g = D[a] + D[b] − 2×c(a, b) is maximal
          hash_t a_max_hash;
          hash_t b_max_hash;
          size_t a_max_idx;
          size_t b_max_idx;
          int    d_max = INT_MIN;
          size_t a_grp_idx = 0;

          // log ("     ");
          // for (size_t i = 0; i < vec_grp_b.size(); i++) {
          //   log ("%3zu ", i);
          // }
          // log ("\n");
          for (auto a_hash: vec_grp_a) {
            size_t a_idx = get_idx_by_hash (a_hash);
            size_t b_grp_idx = 0;
            // log ("%3zu : ", a_grp_idx);
            for (auto b_hash: vec_grp_b) {
              size_t b_idx = get_idx_by_hash (b_hash);
              int g = cell_diff_table[a_grp_idx] + cell_diff_table[b_grp_idx] - 2 * vec_cell_table[a_idx][b_idx];
              if (d_max < g) {
                d_max = g;
                a_max_hash = a_hash;
                b_max_hash = b_hash;
                a_max_idx = a_grp_idx;
                b_max_idx = b_grp_idx;
              }
              // log ("%3d(%zu, %zu) ", g, a_idx, b_idx);
              // log ("%3d ", g);
              b_grp_idx ++;
            }
            // log ("\n");
            a_grp_idx ++;
          }

          // log ("Result === \n");
          // log ("  A_MAX_IDX = %u, B_MAX_IDX = %u, G = %d\n", a_max_hash, b_max_hash, d_max);

          gv.push_back (d_max);
          av.push_back (a_max_hash);
          bv.push_back (b_max_hash);

          // log ("idx: ");
          // for (size_t i = 0; i < gv.size(); i++) {
          //   log ("%3zu  ", i);
          // }
          // log ("\n");
          // log ("gv : "); for (auto g: gv) { log ("%3d, ", g); } log ("\n");
          // log ("av : "); for (auto a: av) { log ("%3d, ", a); } log ("\n");
          // log ("bv : "); for (auto b: bv) { log ("%3d, ", b); } log ("\n");

          vec_grp_a.erase(vec_grp_a.begin() + a_max_idx);
          vec_grp_b.erase(vec_grp_b.begin() + b_max_idx);

        }

        // find k which maximizes g_max, the sum of gv[1], ..., gv[k]
        int gv_total_max = INT_MIN;
        size_t k_max = 0;
        for (size_t k = 0; k < gv.size(); k++) {
          int gv_total = 0;
          for (size_t i = 0; i < k; i++) {
            gv_total += gv[i];
          }
          if (gv_total_max < gv_total) {
            gv_total_max = gv_total;
            k_max = k;
          }
        }
        log ("tmp_cnt = %d, k_max = %zu, g_max = %d\n", tmp_cnt, k_max, gv_total_max);

        if (gv_total_max <= 0) {
          vec_grp_a.swap (av);
          vec_grp_b.swap (bv);
          break;
        }

        vec_grp_a.clear();
        vec_grp_b.clear();

        for (size_t i = 0; i < k_max; i++) {
          vec_grp_a.push_back (bv[i]);
          vec_grp_b.push_back (av[i]);
        }
        for (size_t i = k_max; i < av.size(); i++) {
          vec_grp_a.push_back (av[i]);
        }
        for (size_t i = k_max; i < bv.size(); i++) {
          vec_grp_b.push_back (bv[i]);
        }
      } while (tmp_cnt++ < 100);
    }

    log ("Group A : ");
    for (auto a : vec_grp_a) {
      log ("%08x, ", a);
    }
    log ("\n");
    log ("Group B : ");
    for (auto b : vec_grp_b) {
      log ("%08x, ", b);
    }
    log ("\n");

  }

  size_t get_idx_by_hash (hash_t hash) {
    for (hash_t i = 0; i < m_vec_cell_hash.size(); i++) {
      if (m_vec_cell_hash[i] == hash) {
        return i;
      }
    }
    // __builtin_unreachable();
    assert(false);
  }


  // Search Target Cell Hash Number
  // from_cell_hash : Start Cell Hash Number
  // wire_cell_hash : Wire Hash that connected from from_cell_hash
  // Return : Vector of connected Cells
  std::vector<hash_t> * search_target_cell (RTLIL::Module *module, hash_t from_cell_hash, hash_t wire_cell_hash) {
    std::vector<hash_t> *ret_vec = new std::vector<hash_t> ();

    for (auto cell : module->cells()) {
      if (cell->hash() == from_cell_hash) {
        continue;
      }
      for (auto conn : cell->connections()) {
        if (get_hash (conn.second) == wire_cell_hash) {
          ret_vec->push_back(cell->hash());
        }
      }
    }

    return ret_vec;
  }

  // Get Hash Value from Sigspec
  hash_t get_hash (RTLIL::SigSpec sigspec) {
    if (sigspec.is_chunk()) {
      return get_hash(sigspec.as_chunk());
    } else if (sigspec.is_wire()) {
      return get_hash(sigspec.as_wire());
    } else {
      log ("Unexpected search_sigspec\n");
      exit (1);
    }
  }

  // Get Hash Value from Wire
  hash_t get_hash (RTLIL::Wire *wire) {
    return wire->hash();
  }

  // Get Hash Value from Chunk
  hash_t get_hash (RTLIL::SigChunk chunk) {
    if (chunk.wire) {
      return get_hash (chunk.wire);
    } else {
      return -1;
    }
  }


} Mincut;

PRIVATE_NAMESPACE_END
