#ifndef GRAMMARDATA_H
#define GRAMMARDATA_H

#include "LexData.h"

enum GrammarReturnCode
{
  eGrammarErrorNoError = 0,

  eGrammarErrorDoubleDefinedDataType = 1,
  eGrammarErrorDoubleDefinedVar = 2,

  eGrammarErrorUnknown,
};

enum GrammarNodeType
{
  eGrammarNodeTopNode,   //this node is going to be the top class node in grammar analyzer

  eGrammarNodeBlock,
  eGrammarNodeDataTypeDefine,
  eGrammarNodeVarDefine,
  eGrammarNodeStatment,
};

class GrammarNode {
public:
  GrammarNode();
  virtual ~GrammarNode();

  void setFather(GrammarNode* father);

protected:
  GrammarNode *mFather;
  vector<GrammarNode*> mChildrens;
  uint32 mNodeType;
};

enum DataTypeEnum{
  eDataTypeUnknow,
  eDataTypeBasic,
  eDataTypeStruct,
  eDataTypeEnum,
  eDataTypeClass,
  eDataTypeUnion,
  eDataTypeFunc,
  eDataTypePtr
};

class DataTypeDefine:public GrammarNode {
public:
  DataTypeDefine ();
  virtual ~DataTypeDefine ();

  string getSignature();
  string getKeyWord(int i);

protected:
  vector<string> mKeyWords;
  string mSignature;
  uint32 mDataType;
};

class VarDefine:public GrammarNode {
public:
  VarDefine(){};
  uint32 init (string id, DataTypeDefine* define);
  virtual ~VarDefine ();

  string getId();

private:
  DataTypeDefine* mDataType;
  string mIdentify;
  bool mIsStatic;
  bool mIsVirtual;
  bool mIsConst;
};

enum GrammarBlockType{
  eGrammarBlockTop,
  eGrammarBlockFunc,
  eGrammarBlockWhile,
  eGrammarBlockFor,
  eGrammarBlockIf,
  eGrammarBlockElse
};

class GrammarBlock: public GrammarNode {
public:
  GrammarBlock ();
  virtual ~GrammarBlock ();

  static GrammarBlock* getTopNode();

  uint32 addDataTypeDefine(DataTypeDefine dataType);
  uint32 addVarDefine(VarDefine var);
private:
  map<string,DataTypeDefine> mDataTypeList;
  map<string,VarDefine> mVarList;
};

class BasicDefine:public DataTypeDefine {
public:
  BasicDefine (vector<string> keyWords);
  virtual ~BasicDefine ();

private:
};

class PointerDefine:public DataTypeDefine {
public:
  PointerDefine ();
  virtual ~PointerDefine ();

private:
  DataTypeDefine* mPointerType;
  
};


class FunctionDefine: public DataTypeDefine {
public:
  FunctionDefine ();
  virtual ~FunctionDefine ();

private:
  vector<DataTypeDefine*> inputs;
  DataTypeDefine* output;
};


class ClassDefine: public DataTypeDefine{
public:
  ClassDefine(string word);
  virtual ~ClassDefine();
private:
  map<string,DataTypeDefine*> mFields;
};

class StructDefine: public DataTypeDefine {
public:
  StructDefine ();
  virtual ~StructDefine ();

private:
  map<string,DataTypeDefine*> mFields;
};

class EnumDefine: public DataTypeDefine{
public:
  EnumDefine ();
  virtual ~EnumDefine ();

private:
  map<string, int> mFields;
};

class UnionDefine:public DataTypeDefine {
public:
  UnionDefine ();
  virtual ~UnionDefine ();

private:
  map<string,DataTypeDefine*> mFields;
};


#endif /* end of include guard: GRAMMARDATA_H */