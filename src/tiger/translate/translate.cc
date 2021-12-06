#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  return new Access(level, level->frame_->allocLocal(escape));
}

Level *outermost() {
  std::list<bool> escape;
  return Level::newLevel(nullptr, temp::LabelFactory::NamedLabel("tigermain"),
                         escape);
}

class Cx {
public:
  temp::Label **trues_;
  temp::Label **falses_;
  tree::Stm *stm_;

  Cx(temp::Label **trues, temp::Label **falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() const = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() const = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) const = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return exp_;
  }

  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }

  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
//    auto t = temp::LabelFactory::NewLabel();
//    auto f = temp::LabelFactory::NewLabel();
    auto stm = new tree::CjumpStm(tree::RelOp::NE_OP, exp_,
                                  new tree::ConstExp(0), nullptr, nullptr);
    return {&(stm->true_label_), &(stm->false_label_), stm};
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    errormsg->Error(errormsg->GetTokPos(), "Error: You can't use unCx for nx");
    assert(0);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(temp::Label **trues, temp::Label **falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();

    *cx_.trues_ = t;
    *cx_.falses_ = f;

    return new tree::EseqExp(
        new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
        new tree::EseqExp(
            cx_.stm_,
            new tree::EseqExp(
                new tree::LabelStm(f),
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),
                                                    new tree::ConstExp(0)),
                                  new tree::EseqExp(new tree::LabelStm(t),
                                                    new tree::TempExp(r))))));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(this->UnEx());
  }

  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

ProgTr::ProgTr(std::unique_ptr<absyn::AbsynTree> tr,
               std::unique_ptr<err::ErrorMsg> er)
    : absyn_tree_(std::move(tr)), errormsg_(std::move(er)),
      tenv_(std::make_unique<env::TEnv>()),
      venv_(std::make_unique<env::VEnv>()) {}

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  FillBaseTEnv();
  FillBaseVEnv();
  auto l = outermost();
  auto res = absyn_tree_->Translate(this->venv_.get(), this->tenv_.get(), l,
                                    temp::LabelFactory::NamedLabel("tigermain"),
                                    errormsg_.get());

//  auto stm = frame::procEntryExit1(
//      l->frame_,
//      new tree::MoveStm(
//          new tree::TempExp(frame::X64Frame::regManager.ReturnValue()),
//          res->exp_->UnEx()));
//
//  auto frag = new frame::ProcFrag(stm, l->frame_);
//  frags->PushBack(frag);
}

/**
 *
 * @param target
 * @param curr
 * @return
 */
tree::Exp *staticLink(tr::Level *target, tr::Level *curr) {

  tree::Exp *link =
      new tree::TempExp(frame::X64Frame::regManager.FramePointer());
  while (curr != target) {
    link = curr->frame_->formals.front()->toExp(link);
    curr = curr->parent_;
  }
  return link;
}

Level *Level::newLevel(Level *parent, temp::Label *name,
                       std::list<bool> &formals) {
  formals.push_front(true); // static link
  auto f = frame::X64Frame::newFrame(name, formals);
  auto level = new Level(f, parent);

  return level;
}
} // namespace tr

namespace absyn {

/**
 *
 * @param access the var to be translated
 * @param level  current level
 * @return
 */
tr::Exp *translateSimpleVar(tr::Access *access, tr::Level *level) {
  auto staticLink = tr::staticLink(access->level_, level);
  staticLink = access->access_->toExp(staticLink);
  return new tr::ExExp(staticLink);
}

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::IntTy::Instance();

  auto var_ent = dynamic_cast<env::VarEntry *>(venv->Look(this->sym_));
  exp = translateSimpleVar(var_ent->access_, level);
  ty = var_ent->ty_;
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var = this->var_->Translate(venv, tenv, level, label, errormsg);
  auto tmp_fields = var->ty_->ActualTy();
  auto fields = ((type::RecordTy *)(tmp_fields))->fields_->GetList();

  int num = 0;
  for (const auto &field : fields) {
    if (field->name_ == this->sym_) {
      // match a field
      tree::MemExp *exp = tree::getMemByBaseAndIndex(
          var->exp_->UnEx(), (num * frame::X64Frame::regManager.WordSize()));
      return new tr::ExpAndTy(new tr::ExExp(exp), var->ty_);
    }
    ++num;
  }

  errormsg->Error(errormsg->GetTokPos(), "Not found this field");
  return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto arr = this->var_->Translate(venv, tenv, level, label, errormsg);
  auto sub = this->subscript_->Translate(venv, tenv, level, label, errormsg);

  auto exp = new tr::ExExp((new tree::MemExp(new tree::BinopExp(
      tree::BinOp::PLUS_OP, arr->exp_->UnEx(),
      new tree::BinopExp(
          tree::BinOp::MUL_OP, sub->exp_->UnEx(),
          new tree::ConstExp(frame::X64Frame::regManager.WordSize()))))));

  type::Ty *ty = ((type::ArrayTy *)(arr)->ty_)->ty_->ActualTy();
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return this->var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(this->val_)),
                          type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */ tr::Exp *exp = nullptr;
  type::Ty *ty = type::StringTy::Instance();

  temp::Label *str = temp::LabelFactory::NewLabel();
  auto frag = new frame::StringFrag(str, this->str_);
  frags->PushBack(frag);
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(str)), ty);
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::VoidTy::Instance();

  auto fun = (env::FunEntry *)venv->Look(this->func_);
  if (fun->result_) {
    ty = fun->result_->ActualTy();
  }

  auto exps = new tree::ExpList();
  auto actual_formals = fun->formals_->GetList();
  auto args = this->args_->GetList();
  for (auto &arg : args) {
    auto arg_ = arg->Translate(venv, tenv, level, label, errormsg);
    exps->Append(arg_->exp_->UnEx());
  }

  if (!fun->level_) {
    // external call
    exp = new tr::ExExp(frame::externalCall(this->func_->Name(), exps));
  } else {
    // self-call
    exps->Insert(tr::staticLink(fun->level_->parent_, level));
    exp =
        new tr::ExExp(new tree::CallExp(new tree::NameExp(this->func_), exps));
  }

  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto left = this->left_->Translate(venv, tenv, level, label, errormsg);
  auto right = this->right_->Translate(venv, tenv, level, label, errormsg);

  tr::Exp *exp = nullptr;
  type::Ty *ty = type::IntTy::Instance();

  switch (this->oper_) {
  case Oper::PLUS_OP: {
    exp = new tr::ExExp(new tree::BinopExp(
        tree::BinOp::PLUS_OP, left->exp_->UnEx(), right->exp_->UnEx()));
    break;
  }
  case Oper::MINUS_OP: {
    exp = new tr::ExExp(new tree::BinopExp(
        tree::BinOp::MINUS_OP, left->exp_->UnEx(), right->exp_->UnEx()));
    break;
  }
  case Oper::TIMES_OP: {
    exp = new tr::ExExp(new tree::BinopExp(
        tree::BinOp::MUL_OP, left->exp_->UnEx(), right->exp_->UnEx()));
    break;
  }
  case Oper::DIVIDE_OP: {
    exp = new tr::ExExp(new tree::BinopExp(
        tree::BinOp::DIV_OP, left->exp_->UnEx(), right->exp_->UnEx()));
    break;
  }
  case Oper::LT_OP:
  case Oper::LE_OP:
  case Oper::GT_OP:
  case Oper::GE_OP: {
    tree::CjumpStm *stm;
    switch (this->oper_) {
    case Oper::LT_OP: {
      stm = new tree::CjumpStm(tree::RelOp::LT_OP, left->exp_->UnEx(),
                               right->exp_->UnEx(), nullptr, nullptr);

      break;
    }
    case Oper::LE_OP: {
      stm = new tree::CjumpStm(tree::RelOp::LE_OP, left->exp_->UnEx(),
                               right->exp_->UnEx(), nullptr, nullptr);

      break;
    }
    case Oper::GT_OP: {
      stm = new tree::CjumpStm(tree::RelOp::GT_OP, left->exp_->UnEx(),
                               right->exp_->UnEx(), nullptr, nullptr);

      break;
    }
    case Oper::GE_OP: {
      stm = new tree::CjumpStm(tree::RelOp::GE_OP, left->exp_->UnEx(),
                               right->exp_->UnEx(), nullptr, nullptr);

      break;
    }
    default: {
      assert(0);
    }
    }
    auto true_label = temp::LabelFactory::NewLabel();
    auto false_label = temp::LabelFactory::NewLabel();
    stm->true_label_ = true_label;
    stm->false_label_ = false_label;
    exp = new tr::CxExp(&(stm->true_label_), &(stm->false_label_), stm);
    break;
  }

  case Oper::EQ_OP:
  case Oper::NEQ_OP: {
    tree::CjumpStm *stm;
    switch (this->oper_) {
    case Oper::EQ_OP: {
      auto exps = new tree::ExpList;
      exps->Append(left->exp_->UnEx());
      exps->Append(right->exp_->UnEx());
      if (left->ty_->IsSameType(type::StringTy::Instance())) {
        stm = new tree::CjumpStm(tree::EQ_OP,
                                 frame::externalCall("string_equal", exps),
                                 new tree::ConstExp(1), nullptr, nullptr);
      } else {
        stm = new tree::CjumpStm(tree::EQ_OP, left->exp_->UnEx(),
                                 right->exp_->UnEx(), nullptr, nullptr);
      }
      break;
    }
    case Oper::NEQ_OP: {
      stm = new tree::CjumpStm(tree::NE_OP, left->exp_->UnEx(),
                               right->exp_->UnEx(), nullptr, nullptr);
      break;
    }
    default: {
      assert(0);
    }
    }
    auto true_label = temp::LabelFactory::NewLabel();
    auto false_label = temp::LabelFactory::NewLabel();
    stm->true_label_ = true_label;
    stm->false_label_ = false_label;
    exp = new tr::CxExp(&(stm->true_label_), &(stm->false_label_), stm);
    break;
  }
  default: {
    errormsg->Error(errormsg->GetTokPos(), "undefined ops");
    assert(0);
  }
  }

  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExExp *exp = nullptr;
  auto exps = new tree::ExpList();
  type::Ty *ty = tenv->Look(this->typ_)->ActualTy();

  int num = static_cast<int>(this->fields_->GetList().size());
  auto record_type = (type::RecordTy *)tenv->Look(this->typ_)->ActualTy();
  auto record_list = record_type->fields_->GetList();
  auto fields = this->fields_->GetList();

  for (auto &field : fields) {
    auto tmp = field->exp_->Translate(venv, tenv, level, label, errormsg);
    exps->Append(tmp->exp_->UnEx());
  }

  auto reg = temp::TempFactory::NewTemp();
  auto exp_list = new tree::ExpList();
  exp_list->Append(
      new tree::ConstExp(num * frame::X64Frame::regManager.WordSize()));
  tree::Stm *stm = new tree::MoveStm(
      new tree::TempExp(reg), frame::externalCall("alloc_record", exp_list));

  num = 0;
  auto expressions = exps->GetList();
  for (auto &expr : expressions) {
    auto dst = tree::getMemByBaseAndIndex(
        new tree::TempExp(reg), (num * frame::X64Frame::regManager.WordSize()));
    stm = (new tree::SeqStm(stm, new tree::MoveStm(dst, expr)));
    ++num;
  }

  exp = new tr::ExExp(new tree::EseqExp(stm, new tree::TempExp(reg)));
  return new tr::ExpAndTy(exp, ty);
}

tr::Exp *translateSeq(tr::Exp *left, tr::Exp *right) {

  if (right) {
    return new tr::ExExp(new tree::EseqExp(left->UnNx(), right->UnEx()));
  } else {
    return new tr::ExExp(
        new tree::EseqExp(left->UnNx(), new tree::ConstExp(0)));
  }
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = new tr::ExExp(new tree::ConstExp(0));
  type::Ty *ty = type::IntTy::Instance();

  auto exps = this->seq_->GetList();
  for (auto &expr : exps) {
    auto tmp = expr->Translate(venv, tenv, level, label, errormsg);
    ty = tmp->ty_->ActualTy();
    exp = translateSeq(exp, tmp->exp_);
  }
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::VoidTy::Instance();

  auto dst = this->var_->Translate(venv, tenv, level, label, errormsg);
  auto src = this->exp_->Translate(venv, tenv, level, label, errormsg);

  exp = new tr::NxExp(new tree::MoveStm(dst->exp_->UnEx(), src->exp_->UnEx()));
  return new tr::ExpAndTy(exp, ty);
}

// TODO: 3 Translation about Cx: while, for, if
tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::IntTy::Instance();

  auto test_res = this->test_->Translate(venv, tenv, level, label, errormsg);
  auto then_res = this->then_->Translate(venv, tenv, level, label, errormsg);

  tr::Cx cx = test_res->exp_->UnCx(errormsg);
  auto r = temp::TempFactory::NewTemp();
  auto true_label = temp::LabelFactory::NewLabel();
  auto false_label = temp::LabelFactory::NewLabel();
  auto finally = temp::LabelFactory::NewLabel();
  *cx.trues_ = true_label;
  *cx.falses_ = false_label;

  if (this->elsee_) {
    auto else_res = this->elsee_->Translate(venv, tenv, level, label, errormsg);
    auto finally_vector = new std::vector<temp::Label *>;
    finally_vector->push_back(finally);
    exp = new tr::ExExp(new tree::EseqExp(
        cx.stm_,
        new tree::EseqExp(
            new tree::LabelStm(true_label),
            new tree::EseqExp(
                new tree::MoveStm(new tree::TempExp(r), then_res->exp_->UnEx()),
                new tree::EseqExp(
                    new tree::JumpStm(new tree::NameExp(finally),
                                      finally_vector),
                    new tree::EseqExp(
                        new tree::LabelStm(false_label),
                        (new tree::EseqExp(
                            new tree::MoveStm(new tree::TempExp(r),
                                              else_res->exp_->UnEx()),
                            new tree::EseqExp(
                                new tree::JumpStm(new tree::NameExp(finally),
                                                  finally_vector),
                                new tree::EseqExp(
                                    new tree::LabelStm(finally),
                                    new tree::TempExp(r)))))))))));
  } else {
    exp = new tr::NxExp(new tree::SeqStm(
        cx.stm_,
        new tree::SeqStm(new tree::LabelStm(true_label),
                         new tree::SeqStm(then_res->exp_->UnNx(),
                                          new tree::LabelStm(false_label)))));
  }

  return new tr::ExpAndTy(exp, then_res->ty_->ActualTy());
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::VoidTy::Instance();

  auto done_label = temp::LabelFactory::NewLabel();
  auto test_res = this->test_->Translate(venv, tenv, level, label, errormsg);
  auto body_res =
      this->body_->Translate(venv, tenv, level, done_label, errormsg);

  auto test_label = temp::LabelFactory::NewLabel();
  auto body_label = temp::LabelFactory::NewLabel();
  auto test_cx = test_res->exp_->UnCx(errormsg);
  *test_cx.trues_ = body_label;
  *test_cx.falses_ = done_label;
  auto test_vector = new std::vector<temp::Label *>;
  test_vector->push_back({test_label});
  exp = new tr::NxExp(new tree::SeqStm(
      new tree::LabelStm(test_label),
      new tree::SeqStm(
          test_cx.stm_,
          new tree::SeqStm(
              new tree::LabelStm(body_label),
              new tree::SeqStm(
                  body_res->exp_->UnNx(),
                  new tree::SeqStm(
                      new tree::JumpStm(new tree::NameExp(test_label),
                                        test_vector),
                      new tree::LabelStm(done_label)))))));

  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::VoidTy::Instance();

  auto lo_res = this->lo_->Translate(venv, tenv, level, label, errormsg);
  auto hi_res = this->hi_->Translate(venv, tenv, level, label, errormsg);

  venv->BeginScope();
  venv->Enter(this->var_,
              new env::VarEntry(tr::Access::AllocLocal(level, escape_),
                                lo_res->ty_->ActualTy()));
  auto body_res = this->body_->Translate(venv, tenv, level, label, errormsg);
  venv->EndScope();

  auto dec_list = new absyn::DecList();
  auto exp_list = new absyn::ExpList();
  auto loop_var =
      new absyn::VarDec(0, this->var_, sym::Symbol::UniqueSymbol("int"), lo_);
  auto limit_var = new absyn::VarDec(0, sym::Symbol::UniqueSymbol("limit_var"),
                                     sym::Symbol::UniqueSymbol("int"), hi_);
  loop_var->escape_ = escape_;

  dec_list->Prepend(loop_var);
  dec_list->Prepend(limit_var);

  auto maxint_handler = new IfExp(
      0,
      new OpExp(
          0, absyn::Oper::EQ_OP,
          new absyn::VarExp(0, new absyn::SimpleVar(0, var_)),
          new absyn::VarExp(0, new absyn::SimpleVar(
                                   0, sym::Symbol::UniqueSymbol("limit_var")))),
      new absyn::BreakExp(0), nullptr);

  auto assign_handler = new AssignExp(
      0, new absyn::SimpleVar(0, var_),
      new absyn::OpExp(0, absyn::Oper::PLUS_OP,
                       new absyn::VarExp(0, new absyn::SimpleVar(0, var_)),
                       new absyn::IntExp(0, 1)));

  exp_list->Prepend(assign_handler);
  exp_list->Prepend(maxint_handler);
  exp_list->Prepend(body_);

  auto for2let = new absyn::LetExp(
      0, dec_list,
      new absyn::WhileExp(
          0,
          new absyn::OpExp(
              0, absyn::Oper::LE_OP,
              new absyn::VarExp(0, new absyn::SimpleVar(0, var_)),
              new absyn::VarExp(
                  0, new absyn::SimpleVar(
                         0, sym::Symbol::UniqueSymbol("limit_var")))),
          new absyn::SeqExp(0, exp_list)));

  return for2let->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto jumps = new std::vector<temp::Label *>;
  jumps->push_back(label);
  return new tr::ExpAndTy(
      new tr::NxExp(new tree::JumpStm(new tree::NameExp(label), jumps)),
      type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tree::Exp *exp = nullptr;
  type::Ty *ty = type::VoidTy::Instance();

  static bool first = true;
  bool isMain = false;
  if (first) {
    first = false;
    isMain = true;
  }

  venv->BeginScope();
  tenv->BeginScope();

  tree::Stm *stm = nullptr;
  auto decs = this->decs_->GetList();
  if (!decs.empty()) {
    stm = decs.front()->Translate(venv, tenv, level, label, errormsg)->UnNx();
    decs.pop_front();
    for (auto &dec : decs) {
      auto tmp = dec->Translate(venv, tenv, level, label, errormsg)->UnNx();
      stm = new tree::SeqStm(stm, tmp);
    }
  }

  auto body_res = this->body_->Translate(venv, tenv, level, label, errormsg);

  venv->EndScope();
  tenv->EndScope();

  if (stm) {
    exp = new tree::EseqExp(stm, body_res->exp_->UnEx());
  } else {
    exp = body_res->exp_->UnEx();
  }

  if (isMain) {
    // alloc an procFrag for main function
    isMain = false;
    auto frag = new frame::ProcFrag(new tree::ExpStm(exp), level->frame_);
    frags->PushBack(frag);
  }

  return new tr::ExpAndTy(new tr::ExExp(exp), body_res->ty_->ActualTy());
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = nullptr;
  type::Ty *ty = tenv->Look(this->typ_)->ActualTy();

  auto size_res = this->size_->Translate(venv, tenv, level, label, errormsg);
  auto init_res = this->init_->Translate(venv, tenv, level, label, errormsg);

  auto args = new tree::ExpList();
  args->Append(size_res->exp_->UnEx());
  args->Append(init_res->exp_->UnEx());

  exp = new tr::ExExp(frame::externalCall("init_array", args));
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

/*********************************************************************
 *
 * decs tools
 *
 *********************************************************************/
static type::FieldList *fieldListProducer(env::TEnvPtr tenv,
                                          absyn::FieldList *fieldList) {
  if (!fieldList) {
    return nullptr;
  }

  auto ret = new type::FieldList();
  auto fields = fieldList->GetList();
  for (auto &field : fields) {
    auto ty = tenv->Look(field->typ_);
    ret->Append(new type::Field(field->name_, ty));
  }

  return ret;
}

/*********************************************************************
 *
 * decs translate
 *
 *********************************************************************/
// tricky and hard, jump for a while
tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto funcs = this->functions_->GetList();

  // first time: install all functions and give escape analysis
  for (auto &fun : funcs) {
    // produce formal types
    std::list<bool> formals;
    auto ty_list = new type::TyList();

    // from params get
    auto params = fun->params_->GetList();
    for (auto &para : params) {
      formals.push_back(para->escape_);

      auto ty = tenv->Look(para->typ_)->ActualTy();
      assert(ty);
      ty_list->Append(ty->ActualTy());
    }

    auto new_level = tr::Level::newLevel(level, fun->name_, formals);
    assert(new_level);
    if (fun->result_) {
      auto result_type = tenv->Look(fun->result_);
      venv->Enter(fun->name_, new env::FunEntry(new_level,  (fun->name_), ty_list,
                                                result_type->ActualTy()));
    } else {
      venv->Enter(fun->name_, new env::FunEntry(new_level, fun->name_, ty_list,
                                                type::VoidTy::Instance()));
    }
  }

  // second time: really generate IR for functions
  for (auto &fun : funcs) {
    venv->BeginScope();
    auto fun_ent = (env::FunEntry *)venv->Look(fun->name_);
    auto formals = fun_ent->formals_->GetList();
    auto static_link = fun_ent->level_->frame_->formals;
    auto fields = fun->params_->GetList();

    assert(fields.size() == formals.size());

    static_link.pop_front();
    while (!fields.empty()) {
      assert(!static_link.empty());
      assert(!formals.empty());
      venv->Enter(fields.front()->name_,
                  new env::VarEntry(
                      new tr::Access(fun_ent->level_, static_link.front()),
                      formals.front()->ActualTy()));

      fields.pop_front();
      static_link.pop_front();
      formals.pop_front();
    }
    assert(static_link.empty());
    assert(formals.empty());

    auto res = fun->body_->Translate(venv, tenv, fun_ent->level_,
                                     fun_ent->label_, errormsg);

    // add this fragment
    auto frag = new frame::ProcFrag(
        frame::procEntryExit1(
            fun_ent->level_->frame_,
            new tree::MoveStm(
                new tree::TempExp(frame::X64Frame::regManager.ReturnValue()),
                res->exp_->UnEx())),
        fun_ent->level_->frame_);

    frags->PushBack(frag);
    venv->EndScope();
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto init_res = this->init_->Translate(venv, tenv, level, label, errormsg);

  tr::Access *access = tr::Access::AllocLocal(level, this->escape_);
  venv->Enter(this->var_, new env::VarEntry(access, init_res->ty_->ActualTy()));
  return new tr::NxExp(new tree::MoveStm(
      translateSimpleVar(access, level)->UnEx(), init_res->exp_->UnEx()));
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto tys = this->types_->GetList();

  for (auto &ty : tys) {
    tenv->Enter(ty->name_, new type::NameTy(ty->name_, nullptr));
  }

  for (auto &ty : tys) {
    auto tmp = (type::NameTy *)tenv->Look(ty->name_);
    tmp->ty_ = ty->ty_->Translate(tenv, errormsg);
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

/*********************************************************************
 *
 * ty translate
 *
 *********************************************************************/
type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto res = tenv->Look(this->name_);
  return new type::NameTy(this->name_, res);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto f = fieldListProducer(tenv, this->record_);
  return new type::RecordTy(f);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto res = tenv->Look(this->array_);
  return new type::ArrayTy(res->ActualTy());
}

} // namespace absyn
