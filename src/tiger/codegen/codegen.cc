#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;

} // namespace

namespace cg {

void CodeGen::saveCalleeSavedRegs() {
  callee_saved_r12 = temp::TempFactory::NewTemp();
  callee_saved_r13 = temp::TempFactory::NewTemp();
  callee_saved_r14 = temp::TempFactory::NewTemp();
  callee_saved_r15 = temp::TempFactory::NewTemp();
  callee_saved_rbp = temp::TempFactory::NewTemp();
  callee_saved_rbx = temp::TempFactory::NewTemp();

  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(callee_saved_rbx),
      new temp::TempList(frame::X64RegManager::RBX())));
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(callee_saved_rbp),
      new temp::TempList(frame::X64RegManager::RBP())));
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(callee_saved_r12),
      new temp::TempList(frame::X64RegManager::R12())));
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(callee_saved_r13),
      new temp::TempList(frame::X64RegManager::R13())));
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(callee_saved_r14),
      new temp::TempList(frame::X64RegManager::R14())));
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(callee_saved_r15),
      new temp::TempList(frame::X64RegManager::R15())));
}

void CodeGen::restoreCalleeSavedRegs() {
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(frame::X64RegManager::RBX()),
      new temp::TempList(callee_saved_rbx)));
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(frame::X64RegManager::RBP()),
      new temp::TempList(callee_saved_rbp)));
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(frame::X64RegManager::R12()),
      new temp::TempList(callee_saved_r12)));
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(frame::X64RegManager::R13()),
      new temp::TempList(callee_saved_r13)));
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(frame::X64RegManager::R14()),
      new temp::TempList(callee_saved_r14)));
  assem_instr_->emit(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(frame::X64RegManager::R15()),
      new temp::TempList(callee_saved_r15)));
}

void AssemInstr::emit(assem::Instr *instr) {
  assert(instr_list_);
  instr_list_->Append(instr);
}

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  // eliminate FP
  auto frame_name = frame_->name->Name();
  fs_ = frame_name + "_framesize";

  // make an instr list
  auto tmp = new assem::InstrList();
  assem_instr_ = std::make_unique<AssemInstr>(tmp);

  saveCalleeSavedRegs();
  auto ir_list = traces_->GetStmList()->GetList();
  for (auto &ir : ir_list) {
    ir->Munch(*(this->assem_instr_->GetInstrList()), fs_);
  }
  restoreCalleeSavedRegs();

  // add output active registers
  frame::procEntryExit2(this->assem_instr_->GetInstrList());
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->left_->Munch(instr_list, fs);
  this->right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(this->label_->Name(), this->label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::OperInstr(
      std::string("jmp ").append(this->exp_->name_->Name()), nullptr, nullptr,
      new assem::Targets(this->jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto left = this->left_->Munch(instr_list, fs);
  auto right = this->right_->Munch(instr_list, fs);

  auto temp_list = new temp::TempList();
  temp_list->Append(left);
  temp_list->Append(right);
  instr_list.Append(new assem::OperInstr(
      "cmpq, `s1, `s0", new temp::TempList(left), temp_list, nullptr));

  std::string cjump;
  switch (this->op_) {
  case EQ_OP: {
    cjump = "je ";
    break;
  }
  case NE_OP: {
    cjump = "jne ";
    break;
  }
  case LT_OP:
  case UGE_OP: {
    cjump = "jl ";
    break;
  }
  case GT_OP:
  case ULE_OP: {
    cjump = "jg ";
    break;
  }
  case LE_OP:
  case UGT_OP: {
    cjump = "jle ";
    break;
  }
  case GE_OP:
  case ULT_OP: {
    cjump = "jge ";
    break;
  }
  default:
    break;
  }

  std::vector<temp::Label *> labels;
  labels.push_back(this->true_label_);
  labels.push_back(this->false_label_);

  instr_list.Append(
      new assem::OperInstr(cjump.append(this->true_label_->Name()), nullptr,
                           nullptr, new assem::Targets(&labels)));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto dst_p = this->dst_;
  auto src_p = this->src_;
  assert(dst_p && src_p);
  if (typeid(*dst_p) == typeid(tree::TempExp)) {
    if (typeid(*src_p) == typeid(tree::MemExp)) { // mem to reg

      instr_list.Append(new assem::OperInstr(
          "movq (`s0), `d0", new temp::TempList((dst_p)->Munch(instr_list, fs)),
          new temp::TempList(
              ((tree::MemExp *)src_p)->exp_->Munch(instr_list, fs)),
          nullptr));

    } else if (typeid(*src_p) == typeid(tree::ConstExp)) { // const to reg
      char tmp[256];
      sprintf(tmp, "movq $%d, `d0", ((tree::ConstExp *)(src_p))->consti_);
      instr_list.Append(new assem::OperInstr(
          std::string(tmp), new temp::TempList((dst_p)->Munch(instr_list, fs)),
          nullptr, nullptr));

    } else { // reg to reg

      instr_list.Append(new assem::MoveInstr(
          "movq `s0, `d0", new temp::TempList((dst_p)->Munch(instr_list, fs)),
          new temp::TempList(src_p->Munch(instr_list, fs))));
    }
  } else if (typeid(*dst_p) == typeid(tree::MemExp)) { // only reg to mem
    auto src_list = new temp::TempList();
    src_list->Append(src_p->Munch(instr_list, fs));
    src_list->Append(((tree::MemExp *)dst_p)->exp_->Munch(instr_list, fs));

    instr_list.Append(
        new assem::OperInstr("movq `s0, (`s1)", nullptr, src_list, nullptr));
  } else {
    assert(0);
  }
  //  auto ins = new assem::MoveInstr()
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto ret = temp::TempFactory::NewTemp();
  auto left = this->left_->Munch(instr_list, fs);
  auto right = this->right_->Munch(instr_list, fs);

  switch (this->op_) {
  case PLUS_OP: {
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(ret), new temp::TempList(left)));

    instr_list.Append(new assem::OperInstr("addq `s0, `d0",
                                           new temp::TempList(ret),
                                           new temp::TempList(right), nullptr));
    return ret;
  }
  case MINUS_OP: {
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(ret), new temp::TempList(left)));
    instr_list.Append(new assem::OperInstr("subq `s0, `d0",
                                           new temp::TempList(ret),
                                           new temp::TempList(right), nullptr));
    return ret;
  }
  case MUL_OP: {
    auto dst_regs = new temp::TempList();
    dst_regs->Append(frame::X64RegManager::RAX());
    dst_regs->Append(frame::X64RegManager::RBX());

    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(frame::X64RegManager::RAX()),
        new temp::TempList(left)));

    instr_list.Append(new assem::OperInstr("imulq `s0", dst_regs,
                                           new temp::TempList(right), nullptr));
    instr_list.Append(
        new assem::MoveInstr("movq `s0, `d0", new temp::TempList(ret),
                             new temp::TempList(frame::X64RegManager::RAX())));
    return ret;
  }
  case DIV_OP: {
    auto dst_regs = new temp::TempList();
    dst_regs->Append(frame::X64RegManager::RAX());
    dst_regs->Append(frame::X64RegManager::RBX());

    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(frame::X64RegManager::RAX()),
        new temp::TempList(left)));

    instr_list.Append(new assem::OperInstr(
        "cqto", new temp::TempList(frame::X64RegManager::RDX()),
        new temp::TempList(frame::X64RegManager::RAX()), nullptr));

    instr_list.Append(new assem::OperInstr("idivq `s0", dst_regs,
                                           new temp::TempList(right), nullptr));

    instr_list.Append(
        new assem::MoveInstr("movq `s0, `d0", new temp::TempList(ret),
                             new temp::TempList(frame::X64RegManager::RAX())));
    return ret;
  }
  default:
    assert(0);
  }

  return ret;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto ret = temp::TempFactory::NewTemp();
  auto val = this->exp_->Munch(instr_list, fs);
  instr_list.Append(new assem::OperInstr("movq (`s0), `d0",
                                         new temp::TempList(ret),
                                         new temp::TempList(val), nullptr));
  return ret;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if (this->temp_ != frame::X64Frame::regManager.FramePointer()) {

    return this->temp_;
  } else {
    // replace FP now!
    char tmp[256];
    auto ret = temp::TempFactory::NewTemp();

    sprintf(tmp, "leaq %s(%%rsp), `d0", fs.data());
    instr_list.Append(new assem::OperInstr(
        std::string(tmp), new temp::TempList(ret),
        new temp::TempList(frame::X64Frame::regManager.StackPointer()),
        nullptr));

    return ret;
  }
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->stm_->Munch(instr_list, fs);
  return this->exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // inject Label now!
  char tmp[256];
  auto ret = temp::TempFactory::NewTemp();

  sprintf(tmp, "leaq %s(%%rip), `d0", this->name_->Name().data());
  instr_list.Append(
      new assem::OperInstr(tmp, new temp::TempList(ret), nullptr, nullptr));
  return ret;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  char tmp[256];
  auto ret = temp::TempFactory::NewTemp();

  sprintf(tmp, "movq $%d, `d0", this->consti_);
  instr_list.Append(
      new assem::OperInstr(tmp, new temp::TempList(ret), nullptr, nullptr));
  return ret;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto args = this->args_->MunchArgs(instr_list, fs);
  auto func = dynamic_cast<tree::NameExp *>(this->fun_);
  auto ret = temp::TempFactory::NewTemp();
  char tmp[256];

  sprintf(tmp, "callq %s", func->name_->Name().data());
  instr_list.Append(new assem::OperInstr(
      std::string(tmp), frame::X64Frame::regManager.CallerSaves(), args,
      nullptr));

  int sum = (int)this->args_->GetList().size();
  assert(sum >= 1);
  if(sum > 6) {
    int overSize = (sum - 6) * frame::X64Frame::regManager.WordSize();
    char tmp_[256];
    sprintf(tmp_, "addq $%d, %%rsp", overSize);
    instr_list.Append(new assem::OperInstr(
        std::string(tmp_), new temp::TempList(frame::X64RegManager::RSP()),
        nullptr, nullptr));
  }

  instr_list.Append(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList(ret),
      new temp::TempList(frame::X64Frame::regManager.ReturnValue())));

  return ret;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto ret = new temp::TempList();
  auto args = this->exp_list_;
  // cnt of args
  int num = (int)args.size();

  while (!args.empty()) {
    auto arg = args.back()->Munch(instr_list, fs);
    auto arg_pos = frame::X64RegManager::ARG_nth(num);
    if (arg_pos) {
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                             new temp::TempList(arg_pos),
                                             new temp::TempList(arg)));
      ret->Append(arg_pos);

    } else {
      char tmp[256];
      sprintf(tmp, "subq $%d, %%rsp", frame::X64Frame::regManager.WordSize());
      instr_list.Append(new assem::OperInstr(
          std::string(tmp), new temp::TempList(frame::X64RegManager::RSP()),
          nullptr, nullptr));

      instr_list.Append(new assem::OperInstr(
          "movq `s0, (%rsp)", new temp::TempList(frame::X64RegManager::RSP()),
          new temp::TempList(arg), nullptr));
    }
    args.pop_back();
    --num;
  }

  return ret;
}

} // namespace tree
