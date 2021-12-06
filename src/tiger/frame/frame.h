#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <map>
#include <memory>
#include <string.h>
#include <string>

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"

namespace frame {
/*********************************************************************
 *
 * base class
 *
 *********************************************************************/
class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  temp::Map *temp_map_;

protected:
  std::vector<temp::Temp *> regs_;
};

/*********************************************************************
 *
 * base class
 *
 *********************************************************************/
class Access {
public:
  /* TODO: Put your lab5 code here */
  enum Kind { INFRAME, INREG };

  Kind kind;

  explicit Access(Kind k) : kind(k) {}

  virtual ~Access() = default;

  virtual tree::Exp *toExp(tree::Exp *fp) = 0;
};

class Frame {
  /* TODO: Put your lab5 code here */
public:
  Frame(temp::Label *label) : name(label) {}

  temp::Label *name;

  std::list<frame::Access *> formals;

  std::list<frame::Access *> locals;

  std::list<tree::Stm *> viewShiftOPs;

  int offset;

  int maxArgs;

  virtual Access *allocLocal(bool escape) = 0;
};

/**
 * Fragments
 */
class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
  };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase,
                           bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag *> &GetList() { return frags_; }

private:
  std::list<Frag *> frags_;
};

/*********************************************************************
 *
 * X64 Frame and else platform data structure
 *
 *********************************************************************/
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:
  X64RegManager();

  temp::TempList *Registers() override;

  temp::TempList *ArgRegs() override;

  temp::TempList *CallerSaves() override;

  temp::TempList *CalleeSaves() override;

  temp::TempList *ReturnSink() override;

  int WordSize() override;

  temp::Temp *FramePointer() override;

  temp::Temp *StackPointer() override;

  temp::Temp *ReturnValue() override;

  static temp::Temp *ARG_nth(int num);

  static temp::Temp *RAX();

  static temp::Temp *RBX();

  static temp::Temp *RCX();

  static temp::Temp *RDX();

  static temp::Temp *RSI();

  static temp::Temp *RDI();

  static temp::Temp *RBP();

  static temp::Temp *RSP();

  static temp::Temp *R8();

  static temp::Temp *R9();

  static temp::Temp *R10();

  static temp::Temp *R11();

  static temp::Temp *R12();

  static temp::Temp *R13();

  static temp::Temp *R14();

  static temp::Temp *R15();
};

class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : Access(INFRAME), offset(offset) {}
  /* TODO: Put your lab5 code here */

  tree::Exp *toExp(tree::Exp *fp) override {
    return new tree::MemExp(
        new tree::BinopExp(tree::PLUS_OP, (fp),
                           new tree::ConstExp((this->offset))));
  }
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : Access(INREG), reg(reg) {}
  /* TODO: Put your lab5 code here */

  tree::Exp *toExp(tree::Exp *fp) override {
    return new tree::TempExp(this->reg);
  }
};

/**
 * X64 Frame
 */
class X64Frame : public Frame {
public:
  X64Frame(temp::Label *label, std::list<bool> &list);

  static X64RegManager regManager;

  Access *allocLocal(bool escape) override;

  static Frame *newFrame(temp::Label *name, std::list<bool> &list);
};

tree::Exp *externalCall(const std::string &s, tree::ExpList *args);

// add view shift before function
tree::Stm *procEntryExit1(frame::Frame *frame_, tree::Stm *stm);

assem::InstrList *procEntryExit2(assem::InstrList *body);

assem::Proc *procEntryExit3(frame::Frame *frame1, assem::InstrList *body);

static temp::Temp *rbp = nullptr;

static temp::Temp *rsp = nullptr;

static temp::Temp *rax = nullptr;

static temp::Temp *rdi = nullptr;

static temp::Temp *rsi = nullptr;

static temp::Temp *rdx = nullptr;

static temp::Temp *rcx = nullptr;

static temp::Temp *r8 = nullptr;

static temp::Temp *r9 = nullptr;

static temp::Temp *r10 = nullptr;

static temp::Temp *r11 = nullptr;

static temp::Temp *r12 = nullptr;

static temp::Temp *r13 = nullptr;

static temp::Temp *r14 = nullptr;

static temp::Temp *r15 = nullptr;

static temp::Temp *rbx = nullptr;

} // namespace frame

#endif