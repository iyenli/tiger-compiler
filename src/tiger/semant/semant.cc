#include "tiger/semant/semant.h"
#include "tiger/absyn/absyn.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  root_->SemAnalyze(venv, tenv, 0, errormsg);
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
  auto s = this->var_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (s && typeid(*s) == typeid(type::RecordTy)) {
    auto record = static_cast<type::RecordTy *>(s)->fields_;
    auto l = record->GetList();
    for (auto &item : l) {
      if (item->name_->Name() == this->sym_->Name()) {
        return item->ty_->ActualTy();
      }
    }
    errormsg->Error(this->pos_, "field %s doesn't exist",
                    this->sym_->Name().data());
  } else {
    errormsg->Error(this->pos_, "not a record type");
  }
  return type::IntTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto q = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!q || typeid(*q) != typeid(type::ArrayTy)) {
    errormsg->Error(var_->pos_, "array type required");
  }
  auto p = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!p || !p->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(subscript_->pos_, "number expected here");
  }

  auto arr = static_cast<type::ArrayTy *>(q);
  return arr->ty_;
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return this->var_->SemAnalyze(venv, tenv, labelcount, errormsg);
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
  auto f = venv->Look(this->func_);

  if (!f || typeid(*f) != typeid(env::FunEntry)) {
    errormsg->Error(this->pos_, "undefined function %s",
                    this->func_->Name().data());
    return type::IntTy::Instance();
  }

  auto n = static_cast<env::FunEntry *>(f);
  auto params = n->formals_->GetList();
  auto actual = this->args_->GetList();

  while (!actual.empty() && !params.empty()) {
    auto curr = actual.front()->SemAnalyze(venv, tenv, labelcount, errormsg);
    auto act = params.front();
    if (!curr || typeid(*curr) != typeid(*act)) {
      errormsg->Error(this->pos_, "para type mismatch");
      return type::IntTy::Instance();
    }

    actual.pop_front();
    params.pop_front();
  }

  if(!actual.empty()){
    errormsg->Error(this->pos_, "too many params in function %s", this->func_->Name().data());
    return type::IntTy::Instance();
  }
  if(!params.empty()){
    errormsg->Error(this->pos_, "too little params in function %s", this->func_->Name().data());
    return type::IntTy::Instance();
  }

  return n->result_;
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
  auto s = new type::FieldList();

  auto ty = tenv->Look(this->typ_);
  if (!ty) {
    errormsg->Error(this->pos_, "undefined type rectype");
    return type::IntTy::Instance();
  }

  ty = ty->ActualTy();
  if (typeid(*ty) != typeid(type::RecordTy)) {
    errormsg->Error(this->pos_, "not a record");
    return type::IntTy::Instance();
  }

  auto c = static_cast<type::RecordTy *>(ty);
  auto cl = c->fields_->GetList();
  auto l = this->fields_->GetList();
  if (cl.size() != l.size()) {
    errormsg->Error(this->pos_, "field size not match");
    return type::IntTy::Instance();
  }

  while (!cl.empty()) {
    auto t1 = cl.front();
    auto t2 = l.front();

    if (t1->name_->Name() != t2->name_->Name()) {
      errormsg->Error(this->pos_, "field name not match");
      return type::IntTy::Instance();
    }

    auto t2_ty = t2->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!t2_ty || !t1->ty_) {
      errormsg->Error(this->pos_, "undef type");
      return type::IntTy::Instance();
    }

    auto actual1 = t1->ty_->ActualTy();
    auto actual2 = t2_ty->ActualTy();
    if (typeid(*actual1) != typeid(*actual2)) {
      errormsg->Error(this->pos_, "field type not match");
      return type::IntTy::Instance();
    }

    auto tmp_field = new type::Field(t2->name_, actual2);
    s->Append(tmp_field);

    cl.pop_front();
    l.pop_front();
  }
  return new type::RecordTy(s);
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ret = new type::VoidTy();
  auto l = seq_->GetList();

  for (auto &s : l) {
    ret = s->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return ret;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (typeid(*var_) == typeid(SimpleVar)) {
    auto v = static_cast<SimpleVar *>(var_);
    auto ent = venv->Look(v->sym_);
    if (ent) {
      errormsg->Error(this->pos_, "loop variable can't be assigned");
      return type::IntTy::Instance();
    }
  }
  auto r = exp_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  auto l = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (l) {
    if (typeid(*l) != typeid(*r)) {
      errormsg->Error(this->pos_, "unmatched assign exp");
    }
  }
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*test_ty) != typeid(type::IntTy)) {
    errormsg->Error(test_->pos_, "test exp should be int");
  }
  auto then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (elsee_) {
    auto elsee_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!elsee_ty->IsSameType(then_ty)) {
      errormsg->Error(elsee_->pos_, "then exp and else exp type mismatch");
    }
  } else {
    if (typeid(*then_ty) != typeid(type::VoidTy)) {
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
      return type::VoidTy::Instance();
    }
  }
  return then_ty;
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // TODO: need to handle test value here
  auto ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*ty) != typeid(type::IntTy)) {
    errormsg->Error(test_->pos_, "test exp should be int");
  }
  auto b = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (typeid(*b) != typeid(type::VoidTy)) {
    errormsg->Error(this->pos_, "while body must produce no value");
  }

  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto lt = this->lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto rt = this->hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*lt) != typeid(type::IntTy)) {
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  }
  if (typeid(*rt) != typeid(type::IntTy)) {
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  }

  tenv->BeginScope();
  venv->BeginScope();
  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance()));
  if (this->body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg) !=
      type::VoidTy::Instance()) {
    errormsg->Error(labelcount, "for body can't produce value");
  }
  venv->EndScope();
  tenv->EndScope();

  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (labelcount < 1)
    errormsg->Error(this->pos_, "break is not inside any loop");
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  venv->BeginScope();
  tenv->BeginScope();
  auto s = this->decs_->GetList();
  for (auto &tmp : s) {
    tmp->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  auto ret = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  venv->EndScope();
  tenv->EndScope();
  return ret;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto arr = tenv->Look(typ_);
  auto size_ty = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (!arr || !size_ty || !init_ty) {
    errormsg->Error(this->size_->pos_, "array type undef");
    return type::IntTy::Instance();
  }

  auto actual_arr = arr->ActualTy();
  auto actual_size = size_ty->ActualTy();
  auto actual_init = init_ty->ActualTy();

  if (typeid(*actual_arr) != typeid(type::ArrayTy)) {
    errormsg->Error(this->init_->pos_, "not array type");
  }
  if (typeid(*actual_size) != typeid(type::IntTy)) {
    errormsg->Error(this->size_->pos_, "size of array should be int");
  }

  auto cast = static_cast<type::ArrayTy *>(actual_arr)->ty_;
  if (!cast) {
    errormsg->Error(this->size_->pos_, "array type undef");
    return type::IntTy::Instance();
  }
  if (!cast->ActualTy()->IsSameType(actual_init)) {
    errormsg->Error(this->size_->pos_, "type mismatch");
    return type::IntTy::Instance();
  }

  auto *ret = new type::ArrayTy(actual_arr);
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
  auto funcList = functions_->GetList();
  while(!funcList.empty()){
    auto fr = funcList.front();
    funcList.pop_front();
    for(auto &s: funcList){
      if(s->name_->Name() == fr->name_->Name()){
        errormsg->Error(this->pos_, "two functions have the same name");
      }
    }
  }
  funcList = functions_->GetList();

  // first scan: for header only
  for (auto &fun : funcList) {
    type::Ty *res = nullptr;

    if (fun->result_ != nullptr) { // function
      res = tenv->Look(fun->result_);
      if (!res) {
        errormsg->Error(fun->pos_, "not exist type");
      }
    } else { // procedure
      res = type::VoidTy::Instance();
    }
    auto formals = fun->params_->MakeFormalTyList(tenv, errormsg);
    venv->Enter(fun->name_, new env::FunEntry(formals, res));
  }

  // second scan: for body here
  for (auto &fun : funcList) {
    auto formals = fun->params_->MakeFormalTyList(tenv, errormsg);

    venv->BeginScope();
    auto formal_it = formals->GetList().begin();
    auto param_it = fun->params_->GetList().begin();
    for (; param_it != fun->params_->GetList().end(); formal_it++, param_it++)
      venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it));

    type::Ty *ty = fun->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    venv->EndScope();

    auto res = static_cast<env::FunEntry *>(venv->Look(fun->name_))->result_;
    if (fun->result_ != nullptr) { // function
      if (!res) {                  // not exist, just report once
        return;
      }
      if (typeid(*ty) != typeid(*res)) {
        errormsg->Error(fun->pos_, "wrong return type");
      }
    } else { // procedure
      if (typeid(*ty) != typeid(type::VoidTy)) {
        errormsg->Error(fun->pos_, "procedure returns value");
      }
    }
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if(init_ty == nullptr){
    errormsg->Error(init_->pos_, "init ty is nullptr");
    return;
  }

  if (this->typ_ != nullptr) {
    auto ty = tenv->Look(this->typ_);
    if (ty) {
      if (!ty->IsSameType(init_ty)) {
        errormsg->Error(init_->pos_, "type mismatch");
      }
      auto s = new env::VarEntry(ty->ActualTy());
      venv->Enter(this->var_, s);
    } else {
      errormsg->Error(this->pos_, "illegal var type");
    }
  } else {
    init_ty = init_ty->ActualTy();
    if(typeid(*(init_ty)) == typeid(type::NilTy)){
      errormsg->Error(this->pos_, "init should not be nil without type specified");
    }
    auto s = new env::VarEntry(init_ty);
    venv->Enter(this->var_, s);
  }
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto q = this->types_->GetList();
  while(!q.empty()){
    auto fr = q.front();
    q.pop_front();
    for(auto &s: q){
      if(s->name_->Name() == fr->name_->Name()){
        errormsg->Error(this->pos_, "two types have the same name");
      }
    }
  }

  q = this->types_->GetList();
  // CHECK 1
  for (auto &s : q) {
    tenv->Enter(s->name_, new type::NameTy(s->name_, nullptr));
  }

  for (auto &s : q) {
    auto new_val = s->ty_->SemAnalyze(tenv, errormsg);
    if (new_val) {
      auto tmp = tenv->Look(s->name_);
      auto cast = static_cast<type::NameTy *>(tmp);
      if (typeid(*new_val) == typeid(type::NameTy)) {
        cast->ty_ = new_val;
      } else {
        cast->ty_ = new_val->ActualTy();
      }
    }
  }

  bool isCycle = false;
  for (auto &s : q) {
    auto ty = tenv->Look(s->name_);
    if (typeid(*ty) != typeid(type::NameTy)) {
      continue;
    }

    auto fast = static_cast<type::NameTy *>(ty);
    auto slow = static_cast<type::NameTy *>(ty);
    do {
      if (typeid(*(slow->ty_)) != typeid(type::NameTy))
        break;
      slow = static_cast<type::NameTy *>(slow->ty_);

      if (typeid(*(fast->ty_)) != typeid(type::NameTy))
        break;
      fast = static_cast<type::NameTy *>(fast->ty_);
      if (typeid(*(fast->ty_)) != typeid(type::NameTy))
        break;
      fast = static_cast<type::NameTy *>(fast->ty_);

      if (slow->sym_->Name() == fast->sym_->Name()) {
        isCycle = true;
        break;
      }
    } while (true);

    if (isCycle) {
      errormsg->Error(this->pos_, "illegal type cycle");
      break;
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto v = tenv->Look(this->name_);
  return v;
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto *fl = new type::FieldList();
  auto l = record_->GetList();

  for (auto &s : l) {
    auto tmp_type = tenv->Look(s->typ_);

    if (!tmp_type) {
      errormsg->Error(s->pos_, "undefined type %s", s->typ_->Name().data());
    } else {
      if (typeid(*tmp_type) == typeid(type::NameTy)) {
        auto tmp = new type::Field(s->name_, tmp_type);
        fl->Append(tmp);
      } else {
        auto tmp = new type::Field(s->name_, tmp_type->ActualTy());
        fl->Append(tmp);
      }
    }
  }

  auto ret = new type::RecordTy(fl);
  return ret;
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // TODO: What to return when meeting error
  auto t = tenv->Look(this->array_);
  if (t) {
    return new type::ArrayTy(t->ActualTy());
  }
  errormsg->Error(this->pos_, "undef array type");
  return new type::ArrayTy(type::IntTy::Instance());
}
} // namespace absyn

namespace sem {
void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}
} // namespace sem
