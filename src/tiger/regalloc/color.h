#ifndef TIGER_COMPILER_COLOR_H
#define TIGER_COMPILER_COLOR_H

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"

namespace col {
struct Result {
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}

  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;

  ~Result() = default;
};

class Color {
  /* TODO: Put your lab6 code here */
public:
  Color() = delete;
  Color(frame::Frame *f, assem::InstrList *ir);

  Result *coloring();

private:
  frame::Frame *f;
  assem::InstrList *instrList;

  const static int K = 15;

  void build();
  void makeWorkList();
  void simplify();
  void coalesce();
  void freeze();
  void selectSpill();
  void assignColors();
  void reWriteProgram();
  temp::Map *assignRegisters();

  live::INodeList *simplifyWorklist;
  live::INodeList *freezeWorklist;
  live::INodeList *spillWorklist;
  live::INodeList *spilledNodes;
  live::INodeList *coalescedNodes;
  live::INodeList *coloredNodes;
  live::INodeList *preColorNodes;
  live::INodeList *selectStack;
  live::INodeList *initial;

  live::MoveList *coalescedMoves;
  live::MoveList *constrainedMoves;
  live::MoveList *frozenMoves;
  live::MoveList *worklistMoves;
  live::MoveList *activeMoves;

  live::LiveGraph *live_graph;
  // search temp and corresponding node
  tab::Table<temp::Temp, graph::Node<temp::Temp>> *mapp;

  tab::Table<live::INode, int> *degree;
  tab::Table<live::INode, std::string> *colors;
  tab::Table<live::INode, live::MoveList> *moveLists;
  tab::Table<live::INode, live::INode> *alias;

  temp::TempList *noSpilledNodes;


private:
  void livenessAnalysis();

  void decrementDegree(live::INodePtr pNode);

  void addEdge(live::INodePtr u, live::INodePtr v);

  live::MoveList *nodeMoves(live::INodePtr n);

  bool moveRelated(live::INodePtr n);

  live::INodeListPtr adjacent(live::INodePtr n);

  void enableMoves(live::INodeListPtr nodes);

  live::INodePtr getAlias(live::INodePtr n);

  void addWorkList(live::INodePtr u);

  bool OK(live::INodePtr t, live::INodePtr r);

  bool conservative(live::INodeListPtr nodes);

  void combine(live::INodePtr u, live::INodePtr v);

  void freezeMoves(live::INodePtr u);

  void clearToLastTwo();

  assem::InstrList *getInstrList(std::list<assem::Instr*>);

};
} // namespace col

#endif // TIGER_COMPILER_COLOR_H
