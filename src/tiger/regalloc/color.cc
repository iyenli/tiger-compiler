#include "tiger/regalloc/color.h"
#include "tiger/codegen/codegen.h"

extern frame::RegManager *reg_manager;

namespace col {
/* TODO: Put your lab6 code here */

Color::Color(frame::Frame *f, assem::InstrList *ir)
    : simplifyWorklist(new live::INodeList()),
      freezeWorklist(new live::INodeList()),
      spillWorklist(new live::INodeList()), spilledNodes(new live::INodeList()),
      coalescedNodes(new live::INodeList()), initial(new live::INodeList()),
      coloredNodes(new live::INodeList()), selectStack(new live::INodeList()),
      coalescedMoves(new live::MoveList()), instrList(ir), f(f),
      degree(new tab::Table<live::INode, int>()),
      colors(new tab::Table<live::INode, std::string>()),
      moveLists(new tab::Table<live::INode, live::MoveList>()),
      alias(new tab::Table<live::INode, live::INode>()),
      preColorNodes(new live::INodeList()), live_graph(nullptr),
      noSpilledNodes(new temp::TempList()), mapp(nullptr),
      constrainedMoves(new live::MoveList()), frozenMoves(new live::MoveList()),
      worklistMoves(new live::MoveList()), activeMoves(new live::MoveList()) {}

Result *Color::coloring() {
  while (true) {
    livenessAnalysis();
    build();
    makeWorkList();

    while (!simplifyWorklist->GetList().empty() ||
           !worklistMoves->GetList().empty() ||
           !freezeWorklist->GetList().empty() ||
           !spillWorklist->GetList().empty()) {

      if (!simplifyWorklist->GetList().empty()) {
        simplify();
      } else if (!worklistMoves->GetList().empty()) {
        coalesce();
      } else if (!freezeWorklist->GetList().empty()) {
        freeze();
      } else if (!spillWorklist->GetList().empty()) {
        selectSpill();
      }
    }
    assignColors();

    if (spilledNodes->GetList().empty()) {
      auto color_res = assignRegisters();
      return new Result(color_res, this->instrList);
    } else {
      reWriteProgram();
      clearToLastTwo();
    }
  }
}

void Color::livenessAnalysis() {

  auto flow_graph = new fg::FlowGraphFactory(this->instrList);
  flow_graph->AssemFlowGraph(); // create flow graph

  auto live_graph_ = new live::LiveGraphFactory(flow_graph->GetFlowGraph());
  live_graph_->Liveness();

  auto tmp = live_graph_->GetLiveGraph();
  worklistMoves = tmp.moves;

  live_graph = new live::LiveGraph(tmp.interf_graph, worklistMoves);
  mapp = live_graph_->GetTempNodeMap();
}

// most our work done on liveness analysis, only left moveList and degree table
void Color::build() {
  auto nodes = live_graph->interf_graph->Nodes()->GetList();

  for (auto &node : nodes) {
    degree->Enter(node, new int(node->OutDegree()));
    alias->Enter(node, node);
    auto regInfo =
        frame::X64Frame::regManager.temp_map_->Look(node->NodeInfo());
    colors->Enter(node, regInfo);

    auto moveList = this->worklistMoves->GetList();
    auto toAdd = new live::MoveList();
    for (auto &move : moveList) {
      if (move.first == node || move.second == node) {
        toAdd->Append(move.first, move.second);
      }
    }
    moveLists->Enter(node, toAdd);
  }
}

void Color::addEdge(live::INodePtr u, live::INodePtr v) {
  // Omit adj set and list here, we accomplished them in past section
  if (!u->Adj(v) && u != v) {
    if (colors->Look(u) != nullptr) {
      (*(degree->Look(u)))++;
      live_graph->interf_graph->AddEdge(u, v);
    }
    if (colors->Look(v) != nullptr) {
      (*(degree->Look(v)))++;
      live_graph->interf_graph->AddEdge(v, u);
    }
  }
}

void Color::makeWorkList() {
  // initial is interfere graph nodes
  auto nodes = live_graph->interf_graph->Nodes()->GetList();

  for (auto &node : nodes) {
    if (colors->Look(node)) {
      continue;
    }

    if (*(degree->Look(node)) >= K) {
      spillWorklist->Append(node);
    } else if (moveRelated(node)) {
      freezeWorklist->Append(node);
    } else {
      simplifyWorklist->Append(node);
    }
  }
}

live::INodeListPtr Color::adjacent(live::INodePtr n) {
  return n->Succ()->Diff(selectStack->Union(coalescedNodes));
}

live::MoveList *Color::nodeMoves(live::INodePtr n) {
  auto moveListNode = moveLists->Look(n);
  return moveListNode->Intersect(activeMoves->Union(worklistMoves));
}

bool Color::moveRelated(live::INodePtr n) {
  return !nodeMoves(n)->GetList().empty();
}

void Color::simplify() {
  auto nodes = simplifyWorklist->GetList();

  if (!nodes.empty()) {
    auto toSelect = nodes.front();
    simplifyWorklist->DeleteNode(toSelect);
    selectStack->Append(toSelect);
    auto succeeds = toSelect->Succ()->GetList();
    for (auto &succeed : succeeds) {
      decrementDegree(succeed);
    }
  }
}

void Color::decrementDegree(live::INodePtr pNode) {
  (*degree->Look(pNode))--;
  if (*degree->Look(pNode) == K - 1) {
    auto para = new live::INodeList();
    para->Append(pNode);
    enableMoves(para->Union(adjacent(pNode)));
    spillWorklist->DeleteNode(pNode);
    if (moveRelated(pNode)) {
      freezeWorklist->Append(pNode);
    } else {
      simplifyWorklist->Append(pNode);
    }
  }
}

void Color::enableMoves(live::INodeListPtr nodes) {
  auto node_list = nodes->GetList();
  for (auto &node : node_list) {
    auto moves = nodeMoves(node)->GetList();
    for (auto &move : moves) {
      if (activeMoves->Contain(move.first, move.second)) {
        activeMoves->Delete(move.first, move.second);
        if (!worklistMoves->Contain(move.first, move.second)) {
          worklistMoves->Append(move.first, move.second);
        }
      }
    }
  }
}

void Color::coalesce() {
  if (worklistMoves->GetList().empty()) {
    return;
  }

  auto move = worklistMoves->GetList().front();
  auto x = getAlias(move.first);
  auto y = getAlias(move.second);
  live::INodePtr u, v;
  if (colors->Look(y)) {
    u = y;
    v = x;
  } else {
    u = x;
    v = y;
  }
  worklistMoves->Delete(move.first, move.second);

  if (u == v) {
    if (!coalescedMoves->Contain(x, y)) {
      coalescedMoves->Append(x, y);
    }
    addWorkList(u);
  } else if (colors->Look(v) || u->Succ()->Contain(v)) { // ??
    if (!constrainedMoves->Contain(x, y)) {
      constrainedMoves->Append(x, y);
    }
    addWorkList(u);
    addWorkList(v);
  } else if (colors->Look(u)) {
    // any adj v, OK
    auto adj = adjacent(v)->GetList();
    bool flag = true;
    for (auto &t : adj) {
      if (!OK(t, u)) {
        flag = false;
        break;
      }
    }
    if (flag) {
      if (!coalescedMoves->Contain(x, y)) {
        coalescedMoves->Append(x, y);
      }
      combine(u, v);
      addWorkList(u);
    }
  } else if (!colors->Look(u) &&
             conservative(adjacent(u)->Union(adjacent(v)))) {
    if (!coalescedMoves->Contain(x, y)) {
      coalescedMoves->Append(x, y);
    }
    combine(u, v);
    addWorkList(u);
  } else {
    if (!activeMoves->Contain(x, y)) {
      activeMoves->Append(x, y);
    }
  }
}

void Color::addWorkList(live::INodePtr u) {
  if (!colors->Look(u) && !moveRelated(u) && *degree->Look(u) < K) {
    freezeWorklist->DeleteNode(u);
    simplifyWorklist->Append(u);
  }
}

bool Color::OK(live::INodePtr t, live::INodePtr r) {
  return (*degree->Look(t) < K || colors->Look(t) || t->Succ()->Contain(r));
}

bool Color::conservative(live::INodeListPtr nodes) {
  int k = 0;
  auto node_list = nodes->GetList();
  for (auto &node : node_list) {
    if (*degree->Look(node) >= K) {
      ++k;
    }
  }

  return k < K;
}

live::INodePtr Color::getAlias(live::INodePtr n) {
  if (coalescedNodes->Contain(n)) {
    return getAlias(alias->Look(n));
  } else {
    return n;
  }
}

void Color::combine(live::INodePtr u, live::INodePtr v) {
  if (freezeWorklist->Contain(v)) {
    freezeWorklist->DeleteNode(v);
  } else {
    spillWorklist->DeleteNode(v);
  }

  coalescedNodes->Append(v);
  alias->Set(v, u);
  moveLists->Set(u, moveLists->Look(u)->Union(moveLists->Look(v)));

  auto para = new live::INodeList();
  para->Append(v);
  enableMoves(para);

  auto adjs = adjacent(v)->GetList();
  for (auto &adj : adjs) {
    addEdge(adj, u);
    decrementDegree(adj);
  }

  if (*degree->Look(u) >= K && freezeWorklist->Contain(u)) {
    freezeWorklist->DeleteNode(u);
    spillWorklist->Append(u);
  }
}

void Color::freeze() {
  auto node = freezeWorklist->GetList().front();
  freezeWorklist->DeleteNode(node); // may produce bugs here!!!
  simplifyWorklist->Append(node);
  freezeMoves(node);
}

void Color::freezeMoves(live::INodePtr u) {
  auto moves = nodeMoves(u)->GetList();
  for (auto &move : moves) {
    auto x = move.first;
    auto y = move.second;

    live::INodePtr v = nullptr;
    if (getAlias(y) == getAlias(u)) {
      v = getAlias(x);
    } else {
      v = getAlias(y);
    }

    activeMoves->Delete(move.first, move.second);
    frozenMoves->Append(move.first, move.second);
    if (nodeMoves(v)->GetList().empty() && *degree->Look(v) < K) {
      freezeWorklist->DeleteNode(v);
      simplifyWorklist->Append(v);
    }
  }
}

void Color::selectSpill() {
  auto nodes = spillWorklist->GetList();

  if (nodes.empty()) {
    return;
  }
  int max_degree = 0;
  live::INodePtr toSelect = nullptr;
  for (auto node : nodes) {
    if (live::contain(new temp::TempList(node->NodeInfo()),
                      noSpilledNodes)) { // thank you g-boy!
      continue;
    }
    if (frame::X64Frame::regManager.IsMachineRegister(node->NodeInfo())) {
      continue;
    }
    if (*degree->Look(node) > max_degree) {
      toSelect = node;
      max_degree = *degree->Look(node);
    }
  }

  if (!toSelect) {
    toSelect = nodes.front();
  }

  spillWorklist->DeleteNode(toSelect); // !!
  simplifyWorklist->Append(toSelect);
  freezeMoves(toSelect);
}

void Color::assignColors() {
  auto okColors = new tab::Table<std::string, bool>();
  auto common_regs = frame::X64Frame::regManager.Registers()->GetList();
  for (auto &reg : common_regs) {
    okColors->Enter(frame::X64Frame::regManager.temp_map_->Look(reg),
                    new bool(true));
  }

  while (!selectStack->GetList().empty()) {
    auto n = selectStack->GetList().front();
    selectStack->DeleteNode(n);

    for (auto &reg : common_regs) {
      okColors->Set(frame::X64Frame::regManager.temp_map_->Look(reg),
                    new bool(true));
    }

    auto neighbours = n->Succ()->GetList();
    for (auto &neighbour : neighbours) {
      if (colors->Look(getAlias(neighbour))) {
        okColors->Set(colors->Look(getAlias(neighbour)), new bool(false));
      }
    }

    bool reallySpill = true;
    std::string *realReg = nullptr;
    for (auto &reg : common_regs) {
      if (*(okColors->Look(frame::X64Frame::regManager.temp_map_->Look(reg)))) {
        reallySpill = false;
        realReg = (frame::X64Frame::regManager.temp_map_->Look(reg));
        break;
      }
    }

    if (reallySpill) {
      spilledNodes->Append(n);
    } else {
      if (!coloredNodes->Contain(n)) {
        coloredNodes->Append(n);
      }
      colors->Set(n, realReg);
    }
  }

  auto nodes = coalescedNodes->GetList();
  for (auto &node : nodes) {
    auto actual = colors->Look(getAlias(node));
    colors->Set(node, actual);
  }
}

temp::Map *Color::assignRegisters() {
  auto ret = temp::Map::Empty();
  ret->Enter(frame::X64RegManager::RSP(), new std::string("%rsp"));

  auto nodes = live_graph->interf_graph->Nodes()->GetList();
  for (auto &node : nodes) {
    ret->Enter(node->NodeInfo(), (colors->Look(node)));
  }

  return ret;
}

void Color::reWriteProgram() {
  auto spills = this->spilledNodes->GetList();
  auto instructions = this->instrList->GetList();

  for (; !spills.empty(); spills.pop_front()) {
    auto spill = spills.front();

    // anyway, alloc space for spilled registers in stack
    this->f->offset -= frame::X64Frame::regManager.WordSize();
    auto spilled = spill->NodeInfo();

    for (auto iter = instructions.begin(); iter != instructions.end(); ++iter) {
      temp::TempList *src, *dst;
      auto instr = *iter;

      // Get src and dst temps
      if (typeid(*instr) == typeid(assem::LabelInstr)) {
        continue;
      } else {
        src = instr->Use();
        dst = instr->Def();
      }

      char str[256];
      bool src_contain = src && live::contain(new temp::TempList(spilled), src);
      bool dst_contain = dst && live::contain(new temp::TempList(spilled), dst);

      if (src_contain && dst_contain) {
        auto intermediate = temp::TempFactory::NewTemp();
        noSpilledNodes->Append(intermediate);
        instr->replaceReg(spilled, intermediate);

        sprintf(str, "movq (%s_framesize-%d)(`s0), `d0",
                f->name->Name().c_str(), -f->offset);
        auto newInstr1 = new assem::OperInstr(
            std::string(str), new temp::TempList(intermediate),
            new temp::TempList(frame::X64Frame::regManager.StackPointer()),
            nullptr);
        memset(str, '\0', 256);
        sprintf(str, "movq `s0, (%s_framesize-%d)(`s1)",
                f->name->Name().c_str(), -f->offset);
        auto src_temps = new temp::TempList();
        src_temps->Append(intermediate);
        src_temps->Append(frame::X64Frame::regManager.StackPointer());
        auto newInstr2 =
            new assem::OperInstr(std::string(str), nullptr, src_temps, nullptr);
        instructions.insert(iter, newInstr1);
        ++iter;
        iter = instructions.insert(iter, newInstr2);

      } else if (src_contain) {

        auto intermediate = temp::TempFactory::NewTemp();
        noSpilledNodes->Append(intermediate);
        instr->replaceReg(spilled, intermediate);

        // add instruction
        sprintf(str, "movq (%s_framesize-%d)(`s0), `d0",
                f->name->Name().c_str(), -f->offset);

        auto newInstr = new assem::OperInstr(
            std::string(str), new temp::TempList(intermediate),
            new temp::TempList(frame::X64Frame::regManager.StackPointer()),
            nullptr);

        instructions.insert(iter, newInstr);
      } else if (dst_contain) {

        auto intermediate = temp::TempFactory::NewTemp();
        noSpilledNodes->Append(intermediate);
        // replace equals to spilled node!
        instr->replaceReg(spilled, intermediate);

        // add instruction!!
        sprintf(str, "movq `s0, (%s_framesize-%d)(`s1)",
                f->name->Name().c_str(), -f->offset);
        auto src_temps = new temp::TempList();
        src_temps->Append(intermediate);
        src_temps->Append(frame::X64Frame::regManager.StackPointer());
        auto newInstr =
            new assem::OperInstr(std::string(str), nullptr, src_temps, nullptr);

        ++iter;
        iter = instructions.insert(iter, newInstr);
      }
    }
  }

  this->instrList = getInstrList(instructions);
  clearToLastTwo();
}

void Color::clearToLastTwo() {
  spilledNodes->Clear();
  simplifyWorklist->Clear();
  freezeWorklist->Clear();
  spillWorklist->Clear();
  coalescedNodes->Clear();
  initial->Clear();
  coloredNodes->Clear();
  selectStack->Clear();
  preColorNodes->Clear();

  constrainedMoves = new live::MoveList();
  frozenMoves = new live::MoveList();
  worklistMoves = new live::MoveList();
  activeMoves = new live::MoveList();
  coalescedMoves = new live::MoveList();

  degree = new tab::Table<live::INode, int>();
  colors = new tab::Table<live::INode, std::string>();
  moveLists = new tab::Table<live::INode, live::MoveList>();
  alias = new tab::Table<live::INode, live::INode>();
}

assem::InstrList *Color::getInstrList(std::list<assem::Instr *> instructions) {
  auto new_instructions = new assem::InstrList();
  for (auto &instr : instructions) {
    new_instructions->Append(instr);
  }
  return new_instructions;
}

} // namespace col
