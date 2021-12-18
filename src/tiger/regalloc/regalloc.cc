#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */

// Just replace instruction here!
std::unique_ptr<Result> RegAllocator::TransferResult() {
  // initialize color
  auto color = new col::Color(this->frame, this->assemInstr->GetInstrList());
  // Get program rewritten and color map we need to replace instruction
  auto res = color->coloring();

  auto color_map = res->coloring_;
  auto instr_list = res->il_;
  auto new_list = new assem::InstrList();

  // really remove all move instructions
  for (auto &instr : instr_list->GetList()) {
    if (typeid(*instr) == typeid(assem::MoveInstr)) {
      auto cast = ((assem::MoveInstr *)(instr));
      if (color_map->Look(cast->src_->GetList().front()) ==
          color_map->Look(cast->dst_->GetList().front())) {
        cast->assem_ = "# " + cast->assem_;
      } else {
        new_list->Append(instr);
      }
    } else {
      new_list->Append(instr);
    }
  }

  return std::make_unique<Result>(color_map, new_list);
}

RegAllocator::RegAllocator(frame::Frame *frame_,
                           std::unique_ptr<cg::AssemInstr> assemInstr) {
  frame = frame_;
  this->assemInstr = std::move(assemInstr);
}


} // namespace ra