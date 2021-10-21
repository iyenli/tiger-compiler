#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return stm1->MaxArgs()>stm2->MaxArgs()?stm1->MaxArgs():stm2->MaxArgs();
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  t = stm1->Interp(t);
  return stm2->Interp(t);
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  auto s = exp->Interp(t);
  t = s->t;
  return t->Update(id, s->i);
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
    return exps->NumExps()>exps->MaxArgs()?exps->NumExps():exps->MaxArgs();
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: Shall I print sth really here?
  t = exps->PrintAndInterp(t)->t;
  return t;
}

int PairExpList::NumExps() const {
  return tail->NumExps() + 1;
}

int PairExpList::MaxArgs() const {
  return exp->MaxArgs() + tail->MaxArgs();
}

IntAndTable *PairExpList::Interp(Table *t) {
  return tail->Interp(exp->Interp(t)->t);
}

int LastExpList::NumExps() const {
  return 1;
}

IntAndTable *LastExpList::Interp(Table *t) {
  return exp->Interp(t);
}

int LastExpList::MaxArgs() const {
  return exp->MaxArgs();
}

IntAndTable * IdExp::Interp(Table *t){
  auto *k = new IntAndTable(t->Lookup(id), t);
  return k;
}

IntAndTable *PairExpList::PrintAndInterp(Table *t) {
  auto s = exp->Interp(t);
  std::cout << s->i << ' ';
  return tail->PrintAndInterp(s->t);
}

IntAndTable *LastExpList::PrintAndInterp(Table *t) {
  auto s = exp->Interp(t);
  std::cout << s->i << '\n';
  return s;
}

// No print possibility here
int IdExp::MaxArgs() const{
  return 0;
}

// No print possibility here
IntAndTable * NumExp::Interp(Table *t){
  auto *k = new IntAndTable(num, t);
  return k;
}

int NumExp::MaxArgs() const{
  return 0;
}

int OpExp::MaxArgs() const{
  return left->MaxArgs()>right->MaxArgs()?left->MaxArgs():right->MaxArgs();
}

IntAndTable * OpExp::Interp(Table *t){
  int result = 0;
  IntAndTable *l = left->Interp(t), *r = right->Interp(l->t);
  switch (oper) {
  case MINUS:
    result = l->i - r->i;
    break;
  case PLUS:
    result = l->i + r->i;
    break;
  case DIV:
    if(r->i != 0)
      result = l->i / r->i;
    break;
  case TIMES:
    result = l->i * r->i;
  }
  auto *k = new IntAndTable(result, r->t);
  return k;
}

IntAndTable *EseqExp::Interp(Table *t) {
  t = stm->Interp(t);
  return exp->Interp(t);
}

int EseqExp::MaxArgs() const {
  return stm->MaxArgs()>exp->MaxArgs()?stm->MaxArgs():exp->MaxArgs();
}

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
}  // namespace A
