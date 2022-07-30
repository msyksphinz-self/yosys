#include "kernel/yosys.h"
#include "kernel/satgen.h"
#include "kernel/consteval.h"
#include "kernel/celledges.h"
#include "kernel/macc.h"
#include <algorithm>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct RtlilDump : public Pass {
  RtlilDump() : Pass("rtlil_dump", "My Pass") { }
  void help() override
  {
    //    |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log ("\n");
    log ("    rtlil_dump\n");
    log ("\n");
  }

  void execute(std::vector<std::string> args, RTLIL::Design* design) override
  {
    log ("Hello %lu!\n", args.size());
    auto modules = design->modules();
    for (auto iter = modules.begin(); iter != modules.end(); iter++) {
      log("Module Name = %s\n", (*iter)->name.c_str());
      search_wires (*iter);
      search_cells (*iter);
      search_connections (*iter);
      search_process (*iter);
    }
  }

  void search_wires (RTLIL::Module* module) {
    log("  Wires :\n");
    // auto wires = module->selected_wires();
    auto wires = module->wires();
    for (auto iter = wires.begin(); iter != wires.end(); iter++) {
      log("   Name = %s, width = %d, start_offset = %d\n",
          (*iter)->name.c_str(),
          (*iter)->width,
          (*iter)->start_offset
          );
    }
  }

  void search_cells (RTLIL::Module* module) {
    log("  Cells :\n");
    for (auto cell : module->cells()) {
      log("   Name = %s\n", cell->name.c_str());
      for (auto &conn: cell->connections()) {
        log ("    SigSpec : %s <-> ", conn.first.c_str());
        search_sigspec (conn.second);
        log ("\n");
      }
    }
  }

  void search_connections (RTLIL::Module* module) {
    log("  Connections :\n");
    auto conn = module->connections();
    for (auto iter = conn.begin(); iter != conn.end(); iter++) {
      log("   %s <- %s\n", (*iter).first.as_chunk().wire->name.c_str(),
          (*iter).second.as_chunk().wire->name.c_str());
    }
  }


  void search_process (RTLIL::Module* module) {
    log ("  Process : \n");
    auto procs = module->processes;
    for (auto &proc : procs) {
      log("   %s\n", proc.first.c_str());
      // CaseRule
      search_caserule (&proc.second->root_case);
      // SyncRule
      for (auto &sync: proc.second->syncs) {
        search_syncrule (sync);
      }
    }

  }

  void search_caserule (RTLIL::CaseRule *rule) {
    for (auto &sig : rule->compare) {
      search_sigspec(sig);
    }
    for (auto &act : rule->actions) {
      search_sigsig(act);
    }
    for (auto &sw : rule->switches) {
      search_switchrule(sw);
    }
  }


  void search_sigspec (RTLIL::SigSpec sigspec) {
    if (sigspec.is_chunk()) {
      search_sigchunk(sigspec.as_chunk());
    } else if (sigspec.is_wire()) {
      search_sigwire(sigspec.as_wire());
    }
  }

  void search_sigchunk (RTLIL::SigChunk chunk) {
    if (chunk.wire) {
      log ("chunk %s", chunk.wire->name.c_str());
    }
  }

  void search_sigwire (RTLIL::Wire *wire) {
    log("wire %s", wire->name.c_str());
  }

  void search_sigsig (RTLIL::SigSig sigsig) {
    log("    sigsig: ");
    search_sigspec (sigsig.first);
    log(" <-> ");
    search_sigspec (sigsig.second);
    log("\n");
  }

  void search_switchrule (RTLIL::SwitchRule *switchrule) {
    for (auto &c: switchrule->cases) {
      search_caserule (c);
    }
  }


  void search_syncrule (RTLIL::SyncRule *rule) {
    for (auto &act : rule->actions) {
      search_sigsig(act);
    }

  }

} RtlilDump;


PRIVATE_NAMESPACE_END
