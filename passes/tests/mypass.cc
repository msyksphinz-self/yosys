#include "kernel/yosys.h"
#include "kernel/satgen.h"
#include "kernel/consteval.h"
#include "kernel/celledges.h"
#include "kernel/macc.h"
#include <algorithm>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct MyPass : public Pass {
  MyPass() : Pass("mypass", "My Pass") { }
  void help() override
  {
    //    |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log ("\n");
    log ("    mypass\n");
    log ("\n");
  }

  void execute(std::vector<std::string> args, RTLIL::Design* design) override
  {
    log ("Hello!\n");
    auto modules = design->modules();
    for (auto iter = modules.begin(); iter != modules.end(); iter++) {
      log("Module Name = %s\n", (*iter)->name.c_str());
      search_wires (*iter);
      search_cells (*iter);
    }
  }

  void search_wires (RTLIL::Module* module) {
    log("  Wires :\n");
    // auto wires = module->selected_wires();
    auto wires = module->wires();
    for (auto iter = wires.begin(); iter != wires.end(); iter++) {
      log("   Name = %s\n", (*iter)->name.c_str());
    }
  }

  void search_cells (RTLIL::Module* module) {
    log("  Cells :\n");
    auto wires = module->cells();
    for (auto iter = wires.begin(); iter != wires.end(); iter++) {
      log("   Name = %s\n", (*iter)->name.c_str());
    }
  }




} MyPass;


PRIVATE_NAMESPACE_END
