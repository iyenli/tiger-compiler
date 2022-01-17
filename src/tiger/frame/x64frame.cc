#include "tiger/frame/x64frame.h"
#include "frame.h"

#include <map>

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */

tree::Exp *externalCall(const std::string &s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
                           args);
}

tree::Stm *procEntryExit1(frame::Frame *frame_, tree::Stm *stm) {
  tree::Stm *viewShift = new tree::ExpStm(new tree::ConstExp(0));
  for (auto &stm_ : frame_->viewShiftOPs) {
    viewShift = new tree::SeqStm(stm_, viewShift);
  }

  auto ret = new tree::SeqStm(viewShift, stm);
  return ret;
}

assem::InstrList *procEntryExit2(assem::InstrList *body) {
  auto sinkInstr = new assem::OperInstr("", nullptr, frame::X64Frame::regManager.ReturnSink(), nullptr);
  body->Append(sinkInstr);
  return body;
}

assem::Proc *procEntryExit3(frame::Frame *frame1, assem::InstrList *body) {
  std::string prolog;
  std::string epilog;
  char tmp[256];

  sprintf(tmp, ".set %s_framesize, %d\n", frame1->name->Name().data(),
          -frame1->offset);
  prolog = std::string(tmp);
  memset(tmp, '\0', 256 * sizeof(char));
  sprintf(tmp, "%s:\n", frame1->name->Name().data());
  prolog.append(std::string(tmp));
  memset(tmp, '\0', 256 * sizeof(char));
  sprintf(tmp, "subq $%d, %%rsp\n", -frame1->offset);
  prolog.append(std::string(tmp));

  sprintf(tmp, "addq $%d, %%rsp\n", -frame1->offset);
  epilog = (std::string(tmp));
  epilog.append("retq\n");

  return new assem::Proc(prolog, body, epilog);
}

/*********************************************************************
 *
 * X64 register manager function
 *
 *********************************************************************/

X64RegManager frame::X64Frame::regManager = X64RegManager();

X64RegManager::X64RegManager() {
  if (!(rax)) {
    (rax = temp::TempFactory::NewTemp());
    regs_.push_back(rax);
    auto s_rax = new std::string("%rax");
    temp::Map::Name()->Enter(rax, s_rax);
    this->temp_map_->Enter(rax, s_rax);
  }
  if (!(rbx)) {
    (rbx = temp::TempFactory::NewTemp());
    regs_.push_back(rbx);
    auto s_rbx = new std::string("%rbx");
    temp::Map::Name()->Enter(rbx, s_rbx);
    this->temp_map_->Enter(rbx, s_rbx);
  }
  if (!(rcx)) {
    (rcx = temp::TempFactory::NewTemp());
    regs_.push_back(rcx);
    auto s_rcx = new std::string("%rcx");
    temp::Map::Name()->Enter(rcx, s_rcx);
    this->temp_map_->Enter(rcx, s_rcx);
  }
  if (!(rdx)) {
    (rdx = temp::TempFactory::NewTemp());
    regs_.push_back(rdx);
    auto s_rdx = new std::string("%rdx");
    temp::Map::Name()->Enter(rdx, s_rdx);
    this->temp_map_->Enter(rdx, s_rdx);
  }
  if (!(rsi)) {
    (rsi = temp::TempFactory::NewTemp());
    regs_.push_back(rsi);
    auto s_rsi = new std::string("%rsi");
    temp::Map::Name()->Enter(rsi, s_rsi);
    this->temp_map_->Enter(rsi, s_rsi);
  }
  if (!(rdi)) {
    (rdi = temp::TempFactory::NewTemp());
    auto s_rdi = new std::string("%rdi");
    temp::Map::Name()->Enter(rdi, s_rdi);
    this->temp_map_->Enter(rdi, s_rdi);
  }
  if (!(rbp)) {
    (rbp = temp::TempFactory::NewTemp());
    regs_.push_back(rbp);
    auto s_rbp = new std::string("%rbp");
    temp::Map::Name()->Enter(rbp, s_rbp);
    this->temp_map_->Enter(rbp, s_rbp);
  }
  if (!(rsp)) {
    (rsp = temp::TempFactory::NewTemp());
    regs_.push_back(rsp);
    auto s_rsp = new std::string("%rsp");
    temp::Map::Name()->Enter(rsp, s_rsp);
    this->temp_map_->Enter(rsp, s_rsp);
  }
  if (!(r8)) {
    (r8 = temp::TempFactory::NewTemp());
    regs_.push_back(r8);
    auto s_r8 = new std::string("%r8");
    temp::Map::Name()->Enter(r8, s_r8);
    this->temp_map_->Enter(r8, s_r8);
  }
  if (!(r9)) {
    (r9 = temp::TempFactory::NewTemp());
    regs_.push_back(r9);
    auto s_r9 = new std::string("%r9");
    temp::Map::Name()->Enter(r9, s_r9);
    this->temp_map_->Enter(r9, s_r9);
  }
  if (!(r10)) {
    (r10 = temp::TempFactory::NewTemp());
    regs_.push_back(r10);
    auto s_r10 = new std::string("%r10");
    temp::Map::Name()->Enter(r10, s_r10);
    this->temp_map_->Enter(r10, s_r10);
  }
  if (!(r11)) {
    (r11 = temp::TempFactory::NewTemp());
    regs_.push_back(r11);
    auto s_r11 = new std::string("%r11");
    temp::Map::Name()->Enter(r11, s_r11);
    this->temp_map_->Enter(r11, s_r11);
  }
  if (!(r12)) {
    (r12 = temp::TempFactory::NewTemp());
    regs_.push_back(r12);
    auto s_r12 = new std::string("%r12");
    temp::Map::Name()->Enter(r12, s_r12);
    this->temp_map_->Enter(r12, s_r12);
  }
  if (!(r13)) {
    (r13 = temp::TempFactory::NewTemp());
    regs_.push_back(r13);
    auto s_r13 = new std::string("%r13");
    temp::Map::Name()->Enter(r13, s_r13);
    this->temp_map_->Enter(r13, s_r13);
  }
  if (!(r14)) {
    (r14 = temp::TempFactory::NewTemp());
    regs_.push_back(r14);
    auto s_r14 = new std::string("%r14");
    temp::Map::Name()->Enter(r14, s_r14);
    this->temp_map_->Enter(r14, s_r14);
  }
  if (!(r15)) {
    (r15 = temp::TempFactory::NewTemp());
    regs_.push_back(r15);
    auto s_r15 = new std::string("%r15");
    temp::Map::Name()->Enter(r15, s_r15);
    this->temp_map_->Enter(r15, s_r15);
  }
}

temp::Temp *X64RegManager::RAX() {
  return rax ? rax : (rax = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::RBX() {
  return rbx ? rbx : (rbx = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::RCX() {
  return rcx ? rcx : (rcx = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::RDX() {
  return rdx ? rdx : (rdx = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::RSI() {
  return rsi ? rsi : (rsi = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::RDI() {
  return rdi ? rdi : (rdi = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::RBP() {
  return rbp ? rbp : (rbp = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::RSP() {
  return rsp ? rsp : (rsp = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::R8() {
  return r8 ? r8 : (r8 = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::R9() {
  return r9 ? r9 : (r9 = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::R10() {
  return r10 ? r10 : (r10 = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::R11() {
  return r11 ? r11 : (r11 = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::R12() {
  return r12 ? r12 : (r12 = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::R13() {
  return r13 ? r13 : (r13 = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::R14() {
  return r14 ? r14 : (r14 = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::R15() {
  return r15 ? r15 : (r15 = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::ARG_nth(int num) {
  switch (num) {
  case 1:
    return RDI();
  case 2:
    return RSI();
  case 3:
    return RDX();
  case 4:
    return RCX();
  case 5:
    return R8();
  case 6:
    return R9();
  default:
    return nullptr;
  }
}

temp::Temp *X64RegManager::FramePointer() {
  return rbp ? rbp : (rbp = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::ReturnValue() {
  return rax ? rax : (rax = temp::TempFactory::NewTemp());
}

temp::Temp *X64RegManager::StackPointer() {
  return rsp ? rsp : (rsp = temp::TempFactory::NewTemp());
}

int X64RegManager::WordSize() { return 8; }

// common register
temp::TempList *X64RegManager::Registers() {
  auto ret = new temp::TempList();
  ret->Append(RAX());
  ret->Append(RBX());
  ret->Append(RCX());
  ret->Append(RDX());
  ret->Append(RSI());
  ret->Append(RDI());
  ret->Append(RBP());
  ret->Append(R8());
  ret->Append(R9());
  ret->Append(R10());
  ret->Append(R11());
  ret->Append(R12());
  ret->Append(R13());
  ret->Append(R14());
  ret->Append(R15());
  return ret;
}

temp::TempList *X64RegManager::ArgRegs() {
  auto ret = new temp::TempList();
  ret->Append(RDI());
  ret->Append(RSI());
  ret->Append(RDX());
  ret->Append(RCX());
  ret->Append(R8());
  ret->Append(R9());
  return ret;
}

temp::TempList *X64RegManager::CallerSaves() {
  auto ret = new temp::TempList();
  ret->Append(RAX());
  ret->Append(RCX());
  ret->Append(RDX());
  ret->Append(RSI());
  ret->Append(RDI());
  ret->Append(R8());
  ret->Append(R9());
  ret->Append(R10());
  ret->Append(R11());
  return ret;
}

temp::TempList *X64RegManager::CalleeSaves() {
  auto ret = new temp::TempList();
  ret->Append(RBX());
  ret->Append(RBP());
  ret->Append(R12());
  ret->Append(R13());
  ret->Append(R14());
  ret->Append(R15());
  return ret;
}

temp::TempList *X64RegManager::ReturnSink() {
  auto ret = new temp::TempList();
  ret->Append(RAX());
  ret->Append(RSP());

  return ret;
}

bool X64RegManager::IsMachineRegister(temp::Temp *const &pTemp) {
  auto commons = Registers()->GetList();
  auto ret = false;
  for(auto &common: commons) {
    if(common == pTemp) {
      ret = true;
      break;
    }
  }

  return ret;
}

/*********************************************************************
 *
 * X64Frame function
 *
 *********************************************************************/
Access *X64Frame::allocLocal(bool escape) {
  frame::Access *local;
  if (escape) {
    local = new InFrameAccess(offset);
    offset -= regManager.WordSize();
  } else {
    local = new InRegAccess(temp::TempFactory::NewTemp());
  }
  this->locals.push_back(local);
  return local;
}

// not suit in fact: Not init frame completely!
X64Frame::X64Frame(temp::Label *label, std::list<bool> &list) : Frame(label) {
  this->offset = -8; // depends on implementation
}

Frame *X64Frame::newFrame(temp::Label *name, std::list<bool> &list) {
  Frame *f = new X64Frame(name, list);
  int num = 1;

  // alloc formals
  for (const auto &esc : list) {
    auto local = f->allocLocal(esc);
    f->formals.push_back(local);

    // view shift
    auto arg_reg = X64Frame::regManager.ARG_nth(num);
    if (arg_reg == nullptr) { // in frame args
      f->viewShiftOPs.push_back(new tree::MoveStm(
          local->toExp(
              new tree::TempExp(frame::X64Frame::regManager.FramePointer())),
          new tree::MemExp(new tree::BinopExp(
              tree::BinOp::PLUS_OP,
              new tree::ConstExp((num - 6) *
                                 frame::X64Frame::regManager.WordSize()),
              new tree::TempExp(frame::X64Frame::regManager.FramePointer())))));
    } else {
      f->viewShiftOPs.push_back(
          new tree::MoveStm(local->toExp(new tree::TempExp(
                                frame::X64Frame::regManager.FramePointer())),
                            new tree::TempExp(arg_reg)));
    }

    ++num;
  }

  return f;
}

} // namespace frame