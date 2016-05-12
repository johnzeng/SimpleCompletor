#ifndef GRAMMARANALYZER_H
#define GRAMMARANALYZER_H

#include "LexData.h"
#include "GrammarData.h"
class GrammarAnalyzer {
public:
  GrammarAnalyzer (LexRecList recList);
  virtual ~GrammarAnalyzer ();

  uint32 doAnalyze();

  GrammarBlock *getTopBlock();
protected:
  uint32 blockHeartBeat(int32 index,int32& lastIndex, GrammarBlock* curBlock);

  uint32 handleIdentifier(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleNameSpaceName(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleOriginalNamespaceName(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleNamespaceAlias(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleTypeName(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleClassName(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleEnumName(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleTypedefName(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleSimpleTemplateId(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleTemplateId(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleDeclator(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleDeclatorId(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleIdExpression(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleUnqualifiedId(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleQualifiedId(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleDeclSpecifierSeq(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleDeclSpecifier(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleFunctionBody(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handlePtrDeclarator(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleNonPtrDeclarator(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleParameterAndQualifiers(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleTrailingReturenType(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handlePtrOperator(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleNestNameSpecifier(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleOperatorFunctionId(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleConversionFunctionId(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleLiteralFunctionId(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleDecltypeSpecifier(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleExpression(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleAssignmentExpression(int index, int& lastIndex, GrammarBlock* curBlock);

  uint32 handleFuncDefinition(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleTypeSpecifier(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleTrailingTypeSpecifier(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleClassSpecifier(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleEnumSpecifier(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleSimpleTypeSpecifier(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleThrowExpression(int index, int&lastIndex, GrammarBlock* curBlock);
  uint32 handleLogicOrExpression(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleInitializerClause(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleConditionalExpression(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleLogicAndExpression(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleInclusiveOrExpression(int index, int& lastIndex, GrammarBlock* curBlock);

  uint32 handleEnum(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleEnumId(int index, int& lastIndex, GrammarBlock *curBlock);
  uint32 handleEnumFieldName(int index, int& lastIndex, GrammarBlock* curBlock);

  uint32 handleAttributes(int index, int& lastIndex, GrammarBlock* curBlock);

  //they should be some common method, but I don't sure how to carry the info from invoker now.
  //they may be covered by expected..
//  uint32 handleSemicolon(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 handleLeftBrace(int index, int& lastIndex, GrammarBlock* curBlock);
//  uint32 handleRightBrace(int index, int& lastIndex, GrammarBlock* curBlock);

  uint32 expect(const string& expected,int index, bool oneLine = false);

  bool isLegalVarIdentify(const string& id, GrammarBlock* curBlock);

  //return the statement's type; This will be a long check.... Actually I don't need to care about the right or wrong problem....how about we skip it at first.
  uint32 handleStatement(int index, int& lastIndex, GrammarBlock* curBlock, DataTypeDefine& retDefine);

  uint32 handleStatementLeftBracket(int index, int& lastIndex, GrammarBlock* curBlock, DataTypeDefine& retDefine);
  uint32 expectAConstInt(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 expectAVar(int index, int& lastIndex, GrammarBlock* curBlock, DataTypeDefine& retDefine);
  uint32 getVarCalResult(const string& op,DataTypeDefine* lVar, DataTypeDefine* rVar, DataTypeDefine& retVar);

  uint32 handleCVQualifierSeq(int index, int& lastIndex, GrammarBlock* curBlock);
  uint32 getCVQualifier(int index, int& lastIndex, uint32 &ret);
  uint32 getUnaryOperator(int index, int& lastIndex, uint32 &ret);
  uint32 getConstExpression(int index, int& lastIndex, uint32 &ret);
  uint32 getFunctionSpecifier(int index, int& lastIndex, uint32 &ret);
  uint32 getStorageClassSpecifier(int index, int& lastIndex, uint32 &ret);
  uint32 getAccessSpecifier(int index, int& lastIndex, uint32 &ret);
  uint32 getPureSpecifier(int index, int& lastIndex, uint32 &ret);
  uint32 getVirtSpecifier(int index, int& lastIndex, uint32 &ret);
  uint32 getAssignmentOperator(int index, int& lastIndex, uint32 &ret);

private:
  LexRecList mRecList;

  GrammarBlock mTopBlock;
};

#endif /* end of include guard: GRAMMARANALYZER_H */
