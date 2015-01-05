/*
 * Copyright 2004-2015 Cray Inc.
 * Other additional copyright holders may be indicated within.
 *
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AstDumpToNode.h"

#include "driver.h"
#include "expr.h"
#include "flags.h"
#include "log.h"
#include "stmt.h"
#include "stringutil.h"
#include "symbol.h"
#include "type.h"

#include "WhileDoStmt.h"
#include "DoWhileStmt.h"
#include "CForLoop.h"
#include "ForLoop.h"
#include "ParamForLoop.h"

void AstDumpToNode::view(const char* passName, int passNum)
{
  forv_Vec(ModuleSymbol, module, gModuleSymbols)
  {
    if (log_module[0] == '\0' || strcmp(log_module, module->name) == 0)
    {
      AstDumpToNode logger;

      if (logger.open(module, passName, passNum) == true)
      {
        module->accept(&logger);
        logger.close();
      }
    }
  }
}

AstDumpToNode::AstDumpToNode(FILE* fp)
{
  mPath      = 0;
  mFP        = fp;

  mOffset    = 0;
  mNeedSpace = false;

  mModule    = 0;
}

AstDumpToNode::AstDumpToNode()
{
  mPath      = 0;
  mFP        = 0;

  mOffset    = 0;
  mNeedSpace = false;

  mModule    = 0;
}

AstDumpToNode::~AstDumpToNode()
{
  if (mPath != 0 && mFP != 0)
  {
    close();
  }
}

bool AstDumpToNode::open(ModuleSymbol* mod, const char* passName, int passNum)
{
  char numBuf[4] = { '\0' };

  snprintf(numBuf, 4, "%02d", passNum);

  mPath   = astr(mod->name, "_", numBuf, passName, ".ast");
  mFP     = fopen(astr(log_dir, mPath), "w");

  mModule = mod;

  if (mFP != 0)
  {
    fprintf(mFP, "AST dump for %s after pass %s.\n\n", mod->name, passName);
  }

  return (mFP != 0) ? true : false;
}

bool AstDumpToNode::close()
{
  bool retval = false;

  if (mFP != 0)
  {
    retval = (fclose(mFP) == 0) ? true : false;
    mFP    = 0;
  }

  return retval;
}

/******************************** | *********************************
*                                                                   *
* The visit functions for each concrete class                       *
*                                                                   *
* Note that there are default implementations in AstLogger          *
*                                                                   *
********************************* | ********************************/

bool AstDumpToNode::enterModSym(ModuleSymbol* node)
{
  bool retval = false;

  // chpl_Program embeds a list of all the modules.
  // Skip the contents of the embedded ones.
  if (mModule                                != 0  &&
      strcmp(mModule->name, "chpl__Program") == 0  &&
      strcmp(node->name,    "chpl__Program") != 0)
  {
    fprintf(mFP, "#<ModuleSymbol %12d %s>", node->id, node->name);

    retval  = false;

  }
  else
  {
    fprintf(mFP, "#<ModuleSymbol %12d %s", node->id, node->name);

    mOffset = mOffset + 2;

    newline();

    retval  = true;
  }

  return retval;
}

void AstDumpToNode::exitModSym(ModuleSymbol* node)
{
  if (node->modUseList.n > 0)
  {
    newline();
    fprintf(mFP, "ModUseList:");

    forv_Vec(ModuleSymbol, mod, node->modUseList)
    {
      fprintf(mFP, " %s", mod->name);
    }
  }

  mOffset = mOffset - 2;
  newline();
  fprintf(mFP, ">\n");
}

//
//
//

bool AstDumpToNode::enterBlockStmt(BlockStmt* node)
{
  char heading[128] = { '\0' };
  bool firstTime    = true;

  if (FnSymbol* fn = toFnSymbol(node->parentSymbol))
    if (node == fn->where)
      write(false, "where ", false);

  sprintf(heading, "#<BlockStmt      %12d", node->id);

  write(false, heading, true);

  if (node->blockInfoGet())
  {
    mOffset = mOffset + 2;

    newline();
    write(false, "BlockInfo: ", false);
    mOffset = mOffset + 2;
    node->blockInfoGet()->accept(this);
    mOffset = mOffset - 2;

    mOffset = mOffset - 2;

    newline();
  }

  // Show blockTag bits.
  if (node->blockTag & BLOCK_EXTERN)
    write(true, "extern", true);

  if (node->blockTag & BLOCK_SCOPELESS)
    write(true, "scopeless", true);

  if (node->blockTag & BLOCK_TYPE_ONLY)
    write(true, "type_only", true);

  if (node->blockTag & BLOCK_C_FOR_LOOP)
    write(true, "C_FOR_LOOP", true);

  mOffset = mOffset + 2;

  for_alist(next_ast, node->body)
  {
    newline();

    if (firstTime == true)
      firstTime = false;
    else
      newline();

    next_ast->accept(this);
  }

  if (node->modUses)
  {
    newline();

    write(false, "ModUses: ", false);
    mOffset = mOffset + 2;
    newline();
    node->modUses->accept(this);
    mOffset = mOffset - 2;
  }

  if (node->byrefVars)
  {
    newline();
    write(false, "ByRefVars: ", false);
    mOffset = mOffset + 2;
    newline();
    node->byrefVars->accept(this);
    mOffset = mOffset - 2;
  }

  mOffset = mOffset - 2;

  newline();
  write(false, ">", true);

  return false;
}

//
//
//

bool AstDumpToNode::enterWhileDoStmt(WhileDoStmt* node)
{
  char heading[128] = { '\0' };
  bool firstTime    = true;

  if (FnSymbol* fn = toFnSymbol(node->parentSymbol))
    if (node == fn->where)
      write(false, "where ", false);

  sprintf(heading, "#<WhileDoStmt %12d", node->id);

  write(false, heading, true);

  if (node->condExprGet())
  {
    mOffset = mOffset + 2;

    newline();
    write(false, "CondExpr:", false);
    mOffset = mOffset + 2;
    node->condExprGet()->accept(this);
    mOffset = mOffset - 2;

    mOffset = mOffset - 2;

    newline();
  }

  // Show blockTag bits.
  if (node->blockTag & BLOCK_EXTERN)
    write(false, "extern ", true);

  if (node->blockTag & BLOCK_SCOPELESS)
    write(false, "scopeless ", true);

  if (node->blockTag & BLOCK_TYPE_ONLY)
    write(false, "type_only ", true);

  mOffset = mOffset + 2;

  for_alist(next_ast, node->body)
  {
    if (firstTime == true)
      firstTime = false;
    else
      fprintf(mFP, "\n");

    next_ast->accept(this);
  }

  if (node->modUses)
  {
    newline();
    write(false, "ModUses: ", false);
    node->modUses->accept(this);
  }

  if (node->byrefVars)
  {
    newline();
    write(false, "ByRefVars: ", false);
    node->byrefVars->accept(this);
  }

  mOffset = mOffset - 2;

  newline();
  write(false, ">", true);

  return false;
}

//
//
//

bool AstDumpToNode::enterDoWhileStmt(DoWhileStmt* node)
{
  char heading[128] = { '\0' };
  bool firstTime    = true;

  if (FnSymbol* fn = toFnSymbol(node->parentSymbol))
    if (node == fn->where)
      write(false, "where ", false);

  sprintf(heading, "#<DoWhileStmt %12d", node->id);

  write(false, heading, true);

  if (node->condExprGet())
  {
    mOffset = mOffset + 2;

    newline();
    write(false, "CondExpr:", false);
    mOffset = mOffset + 2;
    node->condExprGet()->accept(this);
    mOffset = mOffset - 2;

    mOffset = mOffset - 2;

    newline();
  }

  // Show blockTag bits.
  if (node->blockTag & BLOCK_EXTERN)
    write(false, "extern ", true);

  if (node->blockTag & BLOCK_SCOPELESS)
    write(false, "scopeless ", true);

  if (node->blockTag & BLOCK_TYPE_ONLY)
    write(false, "type_only ", true);

  mOffset = mOffset + 2;

  for_alist(next_ast, node->body)
  {
    if (firstTime == true)
      firstTime = false;
    else
      fprintf(mFP, "\n");

    next_ast->accept(this);
  }

  if (node->modUses)
  {
    newline();
    write(false, "ModUses: ", false);
    node->modUses->accept(this);
  }

  if (node->byrefVars)
  {
    newline();
    write(false, "ByRefVars: ", false);
    node->byrefVars->accept(this);
  }

  mOffset = mOffset - 2;

  newline();
  write(false, ">", true);

  return false;
}

//
//
//

bool AstDumpToNode::enterCForLoop(CForLoop* node)
{
  char heading[128] = { '\0' };
  bool firstTime    = true;

  if (FnSymbol* fn = toFnSymbol(node->parentSymbol))
    if (node == fn->where)
      write(false, "where ", false);

  sprintf(heading, "#<CForLoop    %12d", node->id);

  write(false, heading, true);

  if (node->initBlockGet())
  {
    mOffset = mOffset + 2;

    newline();
    write(false, "Init:", false);
    mOffset = mOffset + 2;
    node->initBlockGet()->accept(this);
    mOffset = mOffset - 2;

    mOffset = mOffset - 2;

    fprintf(mFP, "\n");
  }

  if (node->testBlockGet())
  {
    mOffset = mOffset + 2;

    newline();
    write(false, "Test:", false);
    mOffset = mOffset + 2;
    node->testBlockGet()->accept(this);
    mOffset = mOffset - 2;

    mOffset = mOffset - 2;

    fprintf(mFP, "\n");
  }

  if (node->incrBlockGet())
  {
    mOffset = mOffset + 2;

    newline();
    write(false, "Incr:", false);
    mOffset = mOffset + 2;
    node->incrBlockGet()->accept(this);
    mOffset = mOffset - 2;

    mOffset = mOffset - 2;

    fprintf(mFP, "\n");
  }

  // Show blockTag bits.
  if (node->blockTag & BLOCK_EXTERN)
    write(false, "extern ", true);

  if (node->blockTag & BLOCK_SCOPELESS)
    write(false, "scopeless ", true);

  if (node->blockTag & BLOCK_TYPE_ONLY)
    write(false, "type_only ", true);

  mOffset = mOffset + 2;

  for_alist(next_ast, node->body)
  {
    if (firstTime == true)
      firstTime = false;
    else
      fprintf(mFP, "\n");

    next_ast->accept(this);
  }

  if (node->modUses)
  {
    newline();
    write(false, "ModUses: ", false);
    node->modUses->accept(this);
  }

  if (node->byrefVars)
  {
    newline();
    write(false, "ByRefVars: ", false);
    node->byrefVars->accept(this);
  }

  mOffset = mOffset - 2;

  newline();
  write(false, ">", true);

  return false;
}


//
//
//

bool AstDumpToNode::enterForLoop(ForLoop* node)
{
  char heading[128] = { '\0' };
  bool firstTime    = true;

  if (FnSymbol* fn = toFnSymbol(node->parentSymbol))
    if (node == fn->where)
      write(false, "where ", false);

  sprintf(heading, "#<ForLoop     %12d", node->id);

  write(false, heading, true);

  if (node->indexGet() != 0 || node->iteratorGet() != 0)
  {
    mOffset = mOffset + 2;
    newline();

    if (node->indexGet() != 0)
    {
      write(false, "Index:", false);
      mOffset = mOffset + 2;
      node->indexGet()->accept(this);
      mOffset = mOffset - 2;
    }

    if (node->iteratorGet() != 0)
    {
      write(false, "Iterator:", false);
      mOffset = mOffset + 2;
      node->iteratorGet()->accept(this);
      mOffset = mOffset - 2;
    }

    mOffset = mOffset - 2;
    newline();
  }

  // Show blockTag bits.
  if (node->blockTag & BLOCK_EXTERN)
    write(false, "extern", true);

  if (node->blockTag & BLOCK_SCOPELESS)
    write(false, "scopeless", true);

  if (node->blockTag & BLOCK_TYPE_ONLY)
    write(false, "type_only", true);

  mOffset = mOffset + 2;

  for_alist(next_ast, node->body)
  {
    if (firstTime == true)
      firstTime = false;
    else
      fprintf(mFP, "\n");

    next_ast->accept(this);
  }

  if (node->modUses)
  {
    newline();
    write(false, "ModUses: ", false);
    node->modUses->accept(this);
  }

  if (node->byrefVars)
  {
    newline();
    write(false, "ByRefVars: ", false);
    node->byrefVars->accept(this);
  }

  mOffset = mOffset - 2;

  newline();
  write(false, ">", true);

  return false;
}


//
//
//

bool AstDumpToNode::enterParamForLoop(ParamForLoop* node)
{
  char heading[128] = { '\0' };
  bool firstTime    = true;

  if (FnSymbol* fn = toFnSymbol(node->parentSymbol))
    if (node == fn->where)
      write(false, "where ", false);

  sprintf(heading, "#<ParamForLoop %12d", node->id);

  write(false, heading, true);

  if (node->indexExprGet())
  {
    mOffset = mOffset + 2;

    newline();
    write(false, "Index Expr:", false);
    mOffset = mOffset + 2;
    node->indexExprGet()->accept(this);
    mOffset = mOffset - 2;

    mOffset = mOffset - 2;

    newline();
  }

  if (node->lowExprGet())
  {
    mOffset = mOffset + 2;

    newline();
    write(false, "Low Expr:", false);
    mOffset = mOffset + 2;
    node->lowExprGet()->accept(this);
    mOffset = mOffset - 2;

    mOffset = mOffset - 2;

    newline();
  }

  if (node->highExprGet())
  {
    mOffset = mOffset + 2;

    newline();
    write(false, "High Expr:", false);
    mOffset = mOffset + 2;
    node->highExprGet()->accept(this);
    mOffset = mOffset - 2;

    mOffset = mOffset - 2;

    newline();
  }

  if (node->strideExprGet())
  {
    mOffset = mOffset + 2;

    newline();
    write(false, "Stride Expr:", false);
    mOffset = mOffset + 2;
    node->strideExprGet()->accept(this);
    mOffset = mOffset - 2;

    mOffset = mOffset - 2;

    newline();
  }

  // Show blockTag bits.
  if (node->blockTag & BLOCK_EXTERN)
    write(false, "extern", true);

  if (node->blockTag & BLOCK_SCOPELESS)
    write(false, "scopeless", true);

  if (node->blockTag & BLOCK_TYPE_ONLY)
    write(false, "type_only", true);

  mOffset = mOffset + 2;

  for_alist(next_ast, node->body)
  {
    if (firstTime == true)
      firstTime = false;
    else
      fprintf(mFP, "\n");

    next_ast->accept(this);
  }

  if (node->modUses)
  {
    newline();
    write(false, "ModUses: ", false);
    node->modUses->accept(this);
  }

  if (node->byrefVars)
  {
    newline();
    write(false, "ByRefVars: ", false);
    node->byrefVars->accept(this);
  }

  mOffset = mOffset - 2;

  newline();
  write(false, ">", true);

  return false;
}


//
//
//

bool AstDumpToNode::enterDefExpr(DefExpr* node)
{
  fprintf(mFP, "#<DefExpr        %12d", node->id);

  mOffset = mOffset + 2;

  if (node->sym)
  {
    newline();
    fprintf(mFP, "sym:      ");

    mOffset = mOffset + 10;
    node->sym->accept(this);
    mOffset = mOffset - 10;
  }

  if (node->exprType)
  {
    newline();
    fprintf(mFP, "exprType: ");

    mOffset = mOffset + 10;
    node->exprType->accept(this);
    mOffset = mOffset - 10;
  }

  if (node->init)
  {
    newline();
    fprintf(mFP, "init:     ");

    mOffset = mOffset + 10;
    node->init->accept(this);
    mOffset = mOffset - 10;
  }

  mOffset = mOffset - 2;
  newline();
  fprintf(mFP, ">");

  return false;
}

//
//
//

bool AstDumpToNode::enterFnSym(FnSymbol* node)
{
  bool firstFormal = true;

  fprintf(mFP, "#<FnSymbol   %12d", node->id);

  mOffset = mOffset + 2;

  if (node->_this && node->_this->defPoint)
  {
    newline();
    fprintf(mFP, "DefPoint:    ");
    mOffset = mOffset + 13;
    ast_symbol(node->_this->type->symbol, false);
    mOffset = mOffset - 13;
  }

  newline();
  fprintf(mFP, "Name:        %s", node->name);

  // Now the return type info
  switch (node->retTag)
  {
    case RET_VALUE:
      break;

    case RET_REF:
      newline();
      write("RetTag:   ref");
      break;

    case RET_PARAM:
      newline();
      write("RetTag:   param");
      break;

    case RET_TYPE:
      newline();
      write("RetTag:   type");
      break;
  }

  if (node->retType && node->retType->symbol)
  {
    newline();
    fprintf(mFP, "RetType:     ");
    mOffset = mOffset + 13;
    node->retType->symbol->accept(this);
    mOffset = mOffset - 13;
  }

  newline();
  write(false, "Formals:     ", false);

  mOffset = mOffset + 13;

  for_alist(next_ast, node->formals)
  {
    if (firstFormal == true)
      firstFormal = false;
    else
      newline();

    next_ast->accept(this);
  }

  mOffset = mOffset - 13;

  if (node->setter)
  {
    newline();
    write(false, "Setter:      ", false);

    mOffset = mOffset + 13;
    node->setter->accept(this);
    mOffset = mOffset - 13;
  }

  if (node->body)
  {
    newline();
    write(false, "Body:        ", false);

    mOffset = mOffset + 13;
    node->body->accept(this);
    mOffset = mOffset - 13;
  }

  if (node->where)
  {
    newline();
    write(false, "Where:       ", false);
    node->where->accept(this);
  }

  if (node->retExprType)
  {
    newline();
    write(false, "RetExprType: ", false);

    mOffset = mOffset + 13;
    node->retExprType->accept(this);
    mOffset = mOffset - 13;
  }

  mOffset = mOffset - 2;

  newline();
  fprintf(mFP, ">");

  return false;
}

//
//
//

bool AstDumpToNode::enterCallExpr(CallExpr* node)
{
  fprintf(mFP, "#<CallExpr       %12d", node->id);

  mOffset = mOffset + 2;
  newline();

  if (FnSymbol* fn = node->isResolved())
  {
    if (fn->hasFlag(FLAG_BEGIN_BLOCK))
      write("begin");

    else if (fn->hasFlag(FLAG_ON_BLOCK))
      write("on");
  }

  if (node->primitive == 0)
    write("call");

  else if (node->isPrimitive(PRIM_RETURN))
    write("return");

  else if (node->isPrimitive(PRIM_YIELD))
    write("yield ");

  else
    write(node->primitive->name);

  if (node->partialTag)
    write("(partial)");

  if (node->baseExpr)
  {
    newline();
    node->baseExpr->accept(this);
  }

  for_alist(next_ast, node->argList)
  {
    newline();
    next_ast->accept(this);
  }

  mOffset = mOffset - 2;
  newline();
  fprintf(mFP, ">");

  return false;
}

//
//
//

bool AstDumpToNode::enterNamedExpr(NamedExpr* node)
{
  fprintf(mFP, "#<NamedExpr");

  fprintf(mFP, "(%s =", node->name);
  mNeedSpace = true;

  return true;
}

void AstDumpToNode::exitNamedExpr(NamedExpr* node)
{
  write(false, ")", true);
}

//
//
//

void AstDumpToNode::visitSymExpr(SymExpr* node)
{
  Symbol* sym = node->var;

  fprintf(mFP, "#<SymExpr           %12d var: ", node->id);
  writeSymbol(sym);
  fprintf(mFP, ">");
}

//
//
//

void AstDumpToNode::visitUsymExpr(UnresolvedSymExpr* node)
{
  fprintf(mFP, "#<UnresolvedSymExpr %12d \"%s\">", node->id, node->unresolved);
}

//
//
//

bool AstDumpToNode::enterCondStmt(CondStmt* node)
{
  fprintf(mFP, "#<CondStmt");

  mOffset = mOffset + 2;
  newline();
  fprintf(mFP, "cond:");

  mOffset = mOffset + 2;
  node->condExpr->accept(this);
  mOffset = mOffset - 2;

  newline();
  fprintf(mFP, "consequent:");

  mOffset = mOffset + 2;
  node->thenStmt->accept(this);
  mOffset = mOffset - 2;

  if (node->elseStmt)
  {
    newline();
    fprintf(mFP, "alternative:");

    mOffset = mOffset + 2;
    node->elseStmt->accept(this);
    mOffset = mOffset - 2;
  }

  mOffset = mOffset - 2;
  newline();
  fprintf(mFP, ">");

  return false;
}


//
//
//

void AstDumpToNode::visitEblockStmt(ExternBlockStmt* node)
{
  fprintf(mFP, "#<ExternBlockStmt");
  fprintf(mFP, "(%s", node->astTagAsString());
  fprintf(mFP, ">");
}

//
//
//

bool AstDumpToNode::enterGotoStmt(GotoStmt* node)
{
  fprintf(mFP, "#<GotoStmt ");

  mOffset = mOffset + 2;
  newline();

  switch (node->gotoTag)
  {
    case GOTO_NORMAL:
      fprintf(mFP, "tag:   goto");
      break;

    case GOTO_BREAK:
      fprintf(mFP, "tag:   break");
      break;

    case GOTO_CONTINUE:
      fprintf(mFP, "tag:   continue");
      break;

    case GOTO_RETURN:
      fprintf(mFP, "tag:   gotoReturn");
      break;

    case GOTO_GETITER_END:
      fprintf(mFP, "tag:   gotoGetiterEnd");
      break;

    case GOTO_ITER_RESUME:
      fprintf(mFP, "tag:   gotoIterResume");
      break;

    case GOTO_ITER_END:
      fprintf(mFP, "tag:   gotoIterEnd");
      break;
  }

  if (SymExpr* label = toSymExpr(node->label))
  {
    if (label->var != gNil)
    {
      newline();
      fprintf(mFP, "label: ");
      mOffset = mOffset + 2;
      ast_symbol(label->var, true);
      mOffset = mOffset - 2;
    }
  }

  mOffset = mOffset - 2;
  newline();
  fprintf(mFP, ">");

  return false;
}

void AstDumpToNode::exitGotoStmt(GotoStmt* node)
{

}

//
//
//

bool AstDumpToNode::enterAggrType(AggregateType* node)
{
  fprintf(mFP, "#<AggregateType %12d %s", node->id, (node->symbol) ? node->symbol->name : "SymbolUnbound");
  mOffset = mOffset + 2;

  return true;
}

void AstDumpToNode::exitAggrType(AggregateType* node)
{
  mOffset = mOffset - 2;
  newline();
  fprintf(mFP, ">");
}

//
//
//

bool AstDumpToNode::enterEnumType(EnumType* node)
{
  fprintf(mFP, "#<EnumType");
  mOffset = mOffset + 2;
  return true;
}

void AstDumpToNode::exitEnumType(EnumType* node)
{
  mOffset = mOffset - 2;
  newline();
  fprintf(mFP, ">");
}

//
//
//

void AstDumpToNode::visitPrimType(PrimitiveType* node)
{
  fprintf(mFP, "#<PrimitiveType %s>", node->symbol->name);
}

//
//
//

bool AstDumpToNode::enterArgSym(ArgSymbol* node)
{
  fprintf(mFP, "#<ArgSymbol  ");

  mOffset = mOffset + 2;
  ast_symbol(node, true);

  if (node->typeExpr)
  {
    newline();
    fprintf(mFP, "typeExpr:     ");
    mOffset = mOffset + 14;
    node->typeExpr->accept(this);
    mOffset = mOffset - 14;
  }

  if (node->defaultExpr)
  {
    newline();
    fprintf(mFP, "defaultExpr:  ");
    mOffset = mOffset + 14;
    node->defaultExpr->accept(this);
    mOffset = mOffset - 14;
  }

  if (node->variableExpr)
  {
    newline();
    fprintf(mFP, "variableExpr: ");
    mOffset = mOffset + 14;
    node->variableExpr->accept(this);
    mOffset = mOffset - 14;
  }

  mOffset = mOffset - 2;
  newline();
  fprintf(mFP, ">");

  return false;
}

//
//
//

void AstDumpToNode::visitEnumSym(EnumSymbol* node)
{
  fprintf(mFP, "#<EnumSymbol>\n");
}

//
//
//

void AstDumpToNode::visitLabelSym(LabelSymbol* node)
{
  fprintf(mFP, "#<LabelSymbol ");
  mOffset = mOffset + 2;
  ast_symbol(node, true);
  mOffset = mOffset - 2;
  fprintf(mFP, ">");
}

//
//
//

bool AstDumpToNode::enterTypeSym(TypeSymbol* node)
{
  fprintf(mFP, "#<TypeSymbol %12d", node->id);
  mOffset = mOffset + 2;

  newline();
  fprintf(mFP, "name:     %s", node->name);
  newline();
  fprintf(mFP, "type:     ");

  mOffset = mOffset + 10;
  node->type->accept(this);
  mOffset = mOffset - 10;

  mOffset = mOffset - 2;

  newline();
  fprintf(mFP, ">");

  return false;
}

//
//
//

void AstDumpToNode::visitVarSym(VarSymbol* node)
{
  writeSymbol(node);
}

/******************************** | *********************************
*                                                                   *
* Helper routines                                                   *
*                                                                   *
********************************* | ********************************/

void AstDumpToNode::writeSymbol(Symbol* sym) const
{
  char          name[1024];
  ModuleSymbol* mod        = sym->getModule();

  if (mod != 0)
  {
    if (false)
      ;

    else if (sym == 0)
      sprintf(name, "??:NULL");

    else if (mod->name == 0 && sym->name == 0)
      sprintf(name, "??:??");

    else if (mod->name != 0 && sym->name == 0)
      sprintf(name, "%s:??", mod->name);

    else if (mod->name == 0 && sym->name != 0)
      sprintf(name, "??:%s", sym->name);

    else
      sprintf(name, "%s.%s", mod->name, sym->name);
  }
  else
  {
    if (sym->name == 0)
      sprintf(name, "NULL:??");

    else
      sprintf(name, "NULL:%s", sym->name);
  }



  if (false)
  {

  }

  else if (isArgSymbol(sym) == true)
  {
    fprintf(mFP, "#<ArgSymbol    %12d name: %-36s", sym->id, name);

    if (sym->type != 0)
    {
      fprintf(mFP, " type:   ");
      writeType(sym->type);
    }

    writeFlags(mFP, sym);
    fprintf(mFP, ">");

  }

  else if (isEnumSymbol(sym) == true)
  {
    fprintf(mFP, "#<EnumSymbol   %12d name: %-36s", sym->id, name);

    if (sym->type != 0)
    {
      fprintf(mFP, " type:   ");
      writeType(sym->type);
    }

    writeFlags(mFP, sym);
    fprintf(mFP, ">");

  }

  else if (isFnSymbol(sym) == true)
  {
    fprintf(mFP, "#<FnSymbol     %12d name: %-36s", sym->id, name);

    if (sym->type != 0)
    {
      fprintf(mFP, " type:   ");
      writeType(sym->type);
    }

    writeFlags(mFP, sym);
    fprintf(mFP, ">");

  }

  else if (isLabelSymbol(sym) == true)
  {
    fprintf(mFP, "#<LabelSymbol  %12d name: %-36s", sym->id, name);

    if (sym->type != 0)
    {
      fprintf(mFP, " type:   ");
      writeType(sym->type);
    }

    writeFlags(mFP, sym);
    fprintf(mFP, ">");

  }

  else if (isModuleSymbol(sym) == true)
  {
    fprintf(mFP, "#<ModuleSymbol %12d name: %-36s", sym->id, name);

    if (sym->type != 0)
    {
      fprintf(mFP, " type:   ");
      writeType(sym->type);
    }

    writeFlags(mFP, sym);
    fprintf(mFP, ">");


  }

  else if (isTypeSymbol(sym) == true)
  {
    fprintf(mFP, "#<TypeSymbol   %12d name: %-36s", sym->id, name);

    if (sym->type != 0)
    {
      fprintf(mFP, " type:   ");
      writeType(sym->type);
    }

    writeFlags(mFP, sym);
    fprintf(mFP, ">");

  }

  else if (VarSymbol* var = toVarSymbol(sym))
  {
    if (var->immediate != 0)
    {
      const size_t bufSize = 128;
      char         imm[bufSize];

      snprint_imm(imm, bufSize, *var->immediate);

      if (is_imag_type(var->type) == true)
      {
        char* tail = strchr(imm, '\0');

        *tail++ = 'i';
        *tail   = '\0';
      }

      fprintf(mFP, "#<VarSymbol         %12d imm:  %-36s", var->id, imm);

      if (sym->type)
      {
        fprintf(mFP, " type:   ");
        writeType(sym->type);
      }

      writeFlags(mFP, sym);

      fprintf(mFP, ">");

    }

    else
    {
      fprintf(mFP, "#<VarSymbol         %12d name: %-36s", var->id, name);

      if (sym->type)
      {
        fprintf(mFP, " type:   ");
        writeType(sym->type);
      }

      writeFlags(mFP, sym);
      fprintf(mFP, ">");
    }
  }
}

void AstDumpToNode::ast_symbol(const char* tag, Symbol* sym, bool def)
{
  fprintf(mFP, "%s ", tag);
  ast_symbol(sym, def);
}


void AstDumpToNode::ast_symbol(Symbol* sym, bool def)
{
  ModuleSymbol* mod = sym->getModule();

  if (def)
  {
    if (ArgSymbol* arg = toArgSymbol(sym))
    {
      newline();

      switch (arg->intent)
      {
        case INTENT_IN:
          fprintf(mFP, "intent:       in arg");
          newline();
          break;
        case INTENT_INOUT:
          fprintf(mFP, "intent:       inout arg");
          newline();
          break;
        case INTENT_OUT:
          fprintf(mFP, "intent:       out arg");
          newline();
          break;
        case INTENT_CONST:
          fprintf(mFP, "intent:       const arg");
          newline();
          break;
        case INTENT_CONST_IN:
          fprintf(mFP, "intent:       const in arg");
          newline();
          break;
        case INTENT_CONST_REF:
          fprintf(mFP, "intent:       const ref arg");
          newline();
          break;
        case INTENT_REF:
          fprintf(mFP, "intent:       ref arg");
          newline();
          break;
        case INTENT_PARAM:
          fprintf(mFP, "intent:       param arg");
          newline();
          break;
        case INTENT_TYPE:
          fprintf(mFP, "intent:       type arg");
          newline();
          break;
        case INTENT_BLANK:
          fprintf(mFP, "intent:       arg");
          newline();
          break;
      }
    }
  }

  if ((mod == 0 || mod->name == 0) && sym->name == 0)
  {
    fprintf(mFP, "name:         %s.%s", "??", "??");
  }

  else if ((mod == 0 || mod->name == 0) && sym->name != 0)
  {
    fprintf(mFP, "name:         %s.%s", "??", sym->name);
  }

  else if (mod->name != 0 && sym->name == 0)
  {
    fprintf(mFP, "name:         %s.%s", mod->name, "??");
  }

  else
  {
    fprintf(mFP, "name:         %s.%s", mod->name, sym->name);
  }

  if (sym->type)
  {
    newline();
    fprintf(mFP, "type:         ");
    writeType(sym->type);
  }

  if (def                &&
      !toTypeSymbol(sym) &&
      sym->type          &&
      sym->type->symbol  &&
      sym->type != dtUnknown)
  {
    newline();
    write(false, "type->symbol: ", false);
    mOffset = mOffset + 14;
    ast_symbol(sym->type->symbol, false);
    mOffset = mOffset - 14;
  }

  if (sym->hasFlag(FLAG_GENERIC))
    write(false, "?", false);

  mNeedSpace = true;
}

void AstDumpToNode::writeType(Type* type) const
{
  if (false)
    ;

  else if (type == 0)
    ;

  else if (PrimitiveType* t = toPrimitiveType(type))
    fprintf(mFP, "#<PrimitiveType %s>", t->symbol->name);

  else if (AggregateType* t = toAggregateType(type))
    fprintf(mFP, "#<AggregateType %s>", t->symbol->name);

  else if (EnumType*      t = toEnumType(type))
    fprintf(mFP, "#<EnumType      %s>", t->symbol->name);

  else
    USR_FATAL("This cannot happen");
}

void AstDumpToNode::write(const char* text)
{
  write(true, text, true);
}

void AstDumpToNode::write(bool spaceBefore, const char* text, bool spaceAfter)
{
  if (spaceBefore == true && mNeedSpace == true)
    fputc(' ', mFP);

  fputs(text, mFP);

  mNeedSpace = spaceAfter;
}

void AstDumpToNode::newline() {
  fputc('\n', mFP);

  for (int i = 0; i < mOffset; ++i)
    fputc(' ', mFP);

  mNeedSpace = false;
}
