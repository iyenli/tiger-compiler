#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  auto instr_list = this->instr_list_->GetList();
  FNodePtr past = nullptr;

  for (const auto &instruction : instr_list) {
    auto node = flowgraph_->NewNode(instruction);
    if (typeid(*instruction) == typeid(assem::LabelInstr)) {
      auto cast = dynamic_cast<assem::LabelInstr *>(instruction);
      label_map_->Enter(cast->label_, node);
    }

    if (past != nullptr) { // just jmp can't link, call and ret can't
      auto info = past->NodeInfo(); // occurs in same frag
      if (typeid(*info) == typeid(assem::OperInstr)) {
        auto cast = dynamic_cast<assem::OperInstr *>(info);
        if (cast->assem_.find("jmp") == std::string::npos) {
          flowgraph_->AddEdge(past, node);
        }
      } else {
        flowgraph_->AddEdge(past, node);
      }
    }
    // update past!
    past = node;
  }

  auto nodes = this->flowgraph_->Nodes()->GetList();
  for (const auto &node : nodes) {
    auto instr = node->NodeInfo();
    if (typeid(*instr) != typeid(assem::OperInstr) ||
        ((assem::OperInstr *)(instr))->jumps_ == nullptr) {
      continue;
    }

    auto targets = ((assem::OperInstr *)(instr))->jumps_->labels_;
    for (const auto &label : *targets) { // jump to labels
      flowgraph_->AddEdge(node, label_map_->Look(label));
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return this->dst_? this->dst_: new temp::TempList();
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return this->dst_? this->dst_: new temp::TempList();
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return this->src_? this->src_:  new temp::TempList();
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return this->src_? this->src_:  new temp::TempList();
}

void OperInstr::replaceReg(temp::Temp *old_, temp::Temp *new_) {
  if(src_) {
    auto new_src = new temp::TempList();
    for(auto &src_reg: src_->GetList()) {
      if(src_reg == old_) {
        new_src->Append(new_);
      } else{
        new_src->Append(src_reg);
      }
    }
    src_ = new_src;
  }

  if(dst_) {
    auto new_dst = new temp::TempList();
    for(auto &dst_reg: dst_->GetList()) {
      if(dst_reg == old_) {
        new_dst->Append(new_);
      } else{
        new_dst->Append(dst_reg);
      }
    }
    dst_ = new_dst;
  }
}

void LabelInstr::replaceReg(temp::Temp *old_, temp::Temp *new_) {}

void MoveInstr::replaceReg(temp::Temp *old_, temp::Temp *new_) {
  if(src_) {
    auto new_src = new temp::TempList();
    for(auto &src_reg: src_->GetList()) {
      if(src_reg == old_) {
        new_src->Append(new_);
      } else{
        new_src->Append(src_reg);
      }
    }
    src_ = new_src;
  }

  if(dst_) {
    auto new_dst = new temp::TempList();
    for(auto &dst_reg: dst_->GetList()) {
      if(dst_reg == old_) {
        new_dst->Append(new_);
      } else{
        new_dst->Append(dst_reg);
      }
    }
    dst_ = new_dst;
  }
}
} // namespace assem
