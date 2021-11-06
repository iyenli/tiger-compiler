#include "tiger/semant/semant.h"
#include "tiger/absyn/absyn.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto s = venv->Look(this->sym_);
  if (s) {
    if (typeid(*s) == typeid(env::VarEntry)) {
      return (static_cast<env::VarEntry *>(s))->ty_->ActualTy();
    } else {
      errormsg->Error(this->pos_, "undefined variable %s",
                      this->sym_->Name().data());
    }
  } else {
    errormsg->Error(this->pos_, "undefined variable %s",
                    this->sym_->Name().data());
  }
  return type::IntTy::Instance();
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto s = this->var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (s) {
    if (typeid(*s) == typeid(type::RecordTy)) {
      auto record = static_cast<type::RecordTy *>(s)->fields_;
      auto l = record->GetList();
      for (auto &item : l) {
        if (item->name_ == this->sym_) {
          return item->ty_->ActualTy();
        }
      }
      errormsg->Error(this->pos_, "No.1, no idea about error report");
    }
    errormsg->Error(this->pos_, "No.2, no idea about error report");
  }
  errormsg->Error(this->pos_, "No.3, no idea about error report");
  // TODO: return what when mistake occurs?
  return type::VoidTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto l = left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  auto r = right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (this->oper_ == absyn::PLUS_OP || this->oper_ == absyn::MINUS_OP ||
      this->oper_ == absyn::TIMES_OP || this->oper_ == absyn::DIVIDE_OP) {
    if (typeid(*l) != typeid(type::IntTy)) {
      errormsg->Error(left_->pos_, "integer required");
    }
    if (typeid(*r) != typeid(type::IntTy)) {
      errormsg->Error(right_->pos_, "integer required");
    }
    return type::IntTy::Instance();
  } else {
    if (!l->IsSameType(r)) {
      errormsg->Error(this->pos_, "same type required");
      return type::IntTy::Instance();
    }
    return type::IntTy::Instance();
  }
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto right = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto left = venv->Look(var_->SemAnalyze(venv, tenv, labelcount, errormsg));
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (this->lo_->SemAnalyze(venv, tenv, labelcount, errormsg) !=
      this->hi_->SemAnalyze(venv, tenv, labelcount, errormsg)) {
    errormsg->Error(labelcount, "");
  }

  if (this->body_->SemAnalyze(venv, tenv, labelcount, errormsg) !=
      type::VoidTy::Instance()) {
    errormsg->Error(labelcount, "");
  }

  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  venv->BeginScope();
  auto s = this->decs_->GetList();
  for (auto &tmp : s) {
    tmp->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  auto ret = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  venv->EndScope();
  return ret;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::ArrayTy *ret = new type::ArrayTy(tenv->Look(typ_));
  return ret;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto l = functions_->GetList();
  for (auto &q : l) {
    env::FunEntry *s = new env::EnvEntry();
    s->formals_ = q->params_->MakeFormalTyList(tenv, errormsg);
    q->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    s->result_ = ((env::VarEntry *)venv->Look(q->result_))->ty_;

    venv->Enter(q->name_, s);
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::VarEntry *s = new env::EnvEntry();
  s->ty_ = typ_;

  venv->Enter(var, s);
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto q = this->types_->GetList();
  for (auto &s : q) {
    tenv->Enter(s->name_, s->ty_);
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}
} // namespace sem
