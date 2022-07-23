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

  void execute(std::vector<std::string> args, RTLIL::Design*) override
  {
    log ("Hello!\n");
  }

} MyPass;


PRIVATE_NAMESPACE_END
