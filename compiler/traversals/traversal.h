#ifndef _TRAVERSAL_H_
#define _TRAVERSAL_H_

#include "../passes/pass.h"

class Stmt;
class Expr;
class Symbol;
class Type;

class Traversal : public Pass {
 public:
  bool processInternalModules;
  bool processTop;
  bool exploreChildStmts;
  bool exploreChildExprs;
  bool exploreChildSymbols;
  bool exploreChildTypes;

  Traversal(bool processThis = true, bool exploreByDefault = true);

  virtual void preProcessStmt(Stmt* stmt);
  virtual void postProcessStmt(Stmt* stmt);
  virtual void preProcessExpr(Expr* expr);
  virtual void postProcessExpr(Expr* expr);
  virtual void preProcessSymbol(Symbol* sym);
  virtual void postProcessSymbol(Symbol* sym);
  virtual void preProcessType(Type* type);
  virtual void postProcessType(Type* type);

  virtual void run(ModuleSymbol* moduleList);
};

#define TRAVERSE(link, traversal, atTop) \
  link->traverse(traversal, atTop)

#define TRAVERSE_LS(link, traversal, atTop) \
  link->traverseList(traversal, atTop)

#define TRAVERSE_DEF(link, traversal, atTop) \
  link->traverseDef(traversal, atTop)

#define TRAVERSE_DEF_LS(link, traversal, atTop) \
  link->traverseDefList(traversal, atTop)

#endif
