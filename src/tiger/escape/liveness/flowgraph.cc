#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  auto instr_list = this->instr_list_->GetList();
  FNodePtr past = nullptr;

  for (const auto &instruction : instr_list) {
    auto node = flowgraph_->NewNode(instruction);

    if (past != nullptr) { // just jmp can't link, call and ret can't
      auto info = past->NodeInfo(); // occurs in same frag
      if (typeid(*info) == typeid(assem::OperInstr)) {
        auto cast = dynamic_cast<assem::OperInstr *>(info);
        if (cast->assem_ != std::string("jmp")) {
          flowgraph_->AddEdge(past, node);
        }
      } else {
        flowgraph_->AddEdge(past, node);
      }
    }

    if (typeid(*instruction) == typeid(assem::LabelInstr)) {
      auto cast = dynamic_cast<assem::LabelInstr *>(instruction);
      label_map_->Enter(cast->label_, node);
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

void LabelInstr::Replace(temp::Temp *oldt, temp::Temp *newt) {
}

void MoveInstr::Replace(temp::Temp *oldt, temp::Temp *newt) {
  if(src_){
    auto src_n = new temp::TempList();
    for(auto &temp : src_->GetList()){
      if(temp==oldt){
        src_n->Append(newt);
      }
      else{
        src_n->Append(temp);
      }
    }
    src_ = src_n;
  }
  if(dst_){
    auto dst_n = new temp::TempList();
    for(auto &temp : dst_->GetList()){
      if(temp==oldt){
        dst_n->Append(newt);
      }
      else{
        dst_n->Append(temp);
      }
    }
    dst_ = dst_n;
  }
}

void OperInstr::Replace(temp::Temp *oldt, temp::Temp *newt) {
  if(src_){
    auto src_n = new temp::TempList();
    for(auto &temp : src_->GetList()){
      if(temp==oldt){
        src_n->Append(newt);
      }
      else{
        src_n->Append(temp);
      }
    }
    src_ = src_n;
  }
  if(dst_){
    auto dst_n = new temp::TempList();
    for(auto &temp : dst_->GetList()){
      if(temp==oldt){
        dst_n->Append(newt);
      }
      else{
        dst_n->Append(temp);
      }
    }
    dst_ = dst_n;
  }
}
} // namespace assem
