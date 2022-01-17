#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (!Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  auto nodes = flowgraph_->Nodes()->GetList();
  for (auto &node : nodes) {
    in_->Enter(node, new temp::TempList());
    out_->Enter(node, new temp::TempList());
  }

  bool stopped = false;
  while (!stopped) {
    stopped = true;
    for (const auto &node : nodes) {
      auto origin_in = in_->Look(node);
      auto origin_out = out_->Look(node);
      auto succ_nodes = node->Succ()->GetList();

      auto out = new temp::TempList();
      for (auto &succeed : succ_nodes) {
        out = union_list(out, in_->Look(succeed));
      }
      auto in = union_list(node->NodeInfo()->Use(),
                           minus_list(out, node->NodeInfo()->Def()));


      in_->Set(node, in);
      out_->Set(node, out);

      if ((has_changed(origin_out, out) || has_changed(origin_in, in))) {
        stopped = false;
      }
    }
  }
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  // add common machine register
  auto common_mr = frame::X64Frame::regManager.Registers()->GetList();
  for (auto &mr : common_mr) {
    auto reg = live_graph_.interf_graph->NewNode(mr);
    temp_node_map_->Enter(mr, reg);
  }

  for (auto &mr1 : common_mr) {
    for (auto &mr2 : common_mr) {
      if (mr1 != mr2) {
        live_graph_.interf_graph->AddEdge(temp_node_map_->Look(mr1),
                                          temp_node_map_->Look(mr2));
      }
    }
  }

  // P178 for basic blocks!
  auto nodes = flowgraph_->Nodes()->GetList();
  for (auto &node : nodes) { // add def and out live to node
    // add out live reg to map and graph
    auto out_live = out_->Look(node);
    for (auto &out : out_live->GetList()) {
      addToLiveNode(out);
    }

    // if def exists here
    auto defs = node->NodeInfo()->Def()->GetList();
    for (auto &def : defs) {
      addToLiveNode(def);
    }
  }

  for (auto &node : nodes) {
    auto instr = node->NodeInfo();
    auto out_live = out_->Look(node);
    auto defs = node->NodeInfo()->Def()->GetList();

    if (typeid(*instr) == typeid(assem::MoveInstr)) {
      // move instr, add all except def
      auto move_src = instr->Use();
//      out_live =  minus_list(union_list(instr->Def(),out_live), move_src);
      out_live =  minus_list((out_live), move_src);
      for (auto &def : defs) {
        for (auto &out : out_live->GetList()) {
          if (def != frame::X64RegManager::RSP() &&
              out != frame::X64RegManager::RSP() && def != out) {
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(def),
                                              temp_node_map_->Look(out));
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(out),
                                              temp_node_map_->Look(def));
          }
        }
      }

      // move instr, add all except def
      auto uses = move_src->GetList();
      for (auto &def : defs) {
        for (auto &use : uses) {
          if (def != frame::X64RegManager::RSP() &&
              use != frame::X64RegManager::RSP()) {
            if (!live_graph_.moves->Contain(temp_node_map_->Look(use),
                                            temp_node_map_->Look(def))) {
              live_graph_.moves->Append(temp_node_map_->Look(use),
                                        temp_node_map_->Look(def));
            }
          }
        }
      }


    } else {
//      out_live = union_list(out_live, instr->Def());
      for (auto &def : defs) {
        for (auto &out : out_live->GetList()) {
          if (def != frame::X64RegManager::RSP() &&
              out != frame::X64RegManager::RSP() && def != out) {
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(def),
                                              temp_node_map_->Look(out));
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(out),
                                              temp_node_map_->Look(def));
          }
        }
      }
    }
  }
}

void LiveGraphFactory::addToLiveNode(temp::Temp *out) {
  if (out == frame::X64RegManager::RSP()) {
    return; // Common reg only
  }
  if (temp_node_map_->Look(out) == nullptr) {
    auto reg = this->live_graph_.interf_graph->NewNode(out);
    temp_node_map_->Enter(out, reg);
  }
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

// tool function
temp::TempList *union_list(temp::TempList *left, temp::TempList *right) {
  assert(left && right);

  auto left_list = left->GetList();
  auto right_list = right->GetList();
  auto ret = new temp::TempList();

  for (auto &t : left_list) {
    ret->Append(t);
  }
  for (auto &t : right_list) {
    bool request_add = true;
    auto curr_list = ret->GetList();
    for (auto &s : curr_list) {
      if (s == t) { // if dup, cancel
        request_add = false;
      }
    }

    if (request_add) {
      ret->Append(t);
    }
  }
  return ret;
}

temp::TempList *minus_list(temp::TempList *left, temp::TempList *right) {
  assert(left && right);

  auto left_list = left->GetList();
  auto right_list = right->GetList();
  auto ret = new temp::TempList();

  for (auto &t : left_list) {
    bool toAdd = true;
    for(auto &s: right_list) {
      if(t == s) {
        toAdd = false;
        break;
      }
    }
    if(toAdd) {
      ret->Append(t);
    }
  }
  return ret;
}

bool has_changed(temp::TempList *left, temp::TempList *right) {
  assert(left && right);
  return !(contain(left, right) && contain(right, left));
}

/**
 * if right temp list contains left temp list
 * return true
 */
bool contain(temp::TempList *left, temp::TempList *right) {
  assert(left && right);

  auto left_list = left->GetList();
  auto right_list = right->GetList();

  for (auto &l : left_list) {
    bool exist = false;
    for (auto &r : right_list) {
      if (l == r) {
        exist = true;
        break;
      }
    }
    if (!exist) {
      return false;
    }
  }
  return true;
}

} // namespace live