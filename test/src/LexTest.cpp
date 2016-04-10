#include "Lex.h"
#include "CmdInputFactor.h"
#include <gtest/gtest.h>
#include "JZLogger.h"
//this will be a large test, will have lots of test here, I don't sure if we should seperate this into many parts

//this is lib test entryp, I will use this file as entry
void globalInitAtBegin();

//cover test: comment, basic input
TEST(Lex, simpleInput){
  //this is really a strong test, so don't do it for more than one file every time
  int argc = 2;
  char argv0[128] = {0},argv1[128] = {0};
  strcpy(argv0,"tester");
  strcpy(argv1,"./test/TestSet/simple");
  char* argv[2] = {argv0,argv1};

	//init
	globalInitAtBegin();

	JZSetLoggerLevel(JZ_LOG_TEST);

	//analyze command line input
	CmdInputFactor::getInstance()->analyze(argc, argv);

	//now begin to analyze the files input from command line
	string toCompileFile = CmdInputFactor::getInstance()->getNextFile();

  Lex lex;
  lex.analyzeAFile(toCompileFile);
  LexRecList recList = lex.getRecList();
  EXPECT_EQ(3, recList.size());
  ASSERT_STREQ("int", recList[0].word.c_str());
  ASSERT_STREQ("a", recList[1].word.c_str());
  ASSERT_STREQ(";", recList[2].word.c_str());

}

//cover case: simple define, basic input
TEST(Lex, macroTest1){
  //this is really a strong test, so don't do it for more than one file every time
  int argc = 2;
  char argv0[128] = {0},argv1[128] = {0};
  strcpy(argv0,"tester");
  strcpy(argv1,"./test/TestSet/macro_test_1");
  char* argv[2] = {argv0,argv1};

	JZSetLoggerLevel(JZ_LOG_TEST);

	//analyze command line input
	CmdInputFactor::getInstance()->analyze(argc, argv);

	//now begin to analyze the files input from command line
	string toCompileFile = CmdInputFactor::getInstance()->getNextFile();

  Lex lex;
  lex.analyzeAFile(toCompileFile);
  LexRecList recList = lex.getRecList();
  EXPECT_EQ(5, recList.size());
  ASSERT_STREQ("main", recList[0].word.c_str());
  ASSERT_STREQ("(", recList[1].word.c_str());
  ASSERT_STREQ("1", recList[2].word.c_str());
  ASSERT_STREQ(")", recList[3].word.c_str());
  ASSERT_STREQ(";", recList[4].word.c_str());

}

//cover case: macro with input parameter, commented define
TEST(Lex, macroTest2){
  //this is really a strong test, so don't do it for more than one file every time
  int argc = 2;
  char argv0[128] = {0},argv1[128] = {0};
  strcpy(argv0,"tester");
  strcpy(argv1,"./test/TestSet/macro_test_2");
  char* argv[2] = {argv0,argv1};

	JZSetLoggerLevel(JZ_LOG_DEBUG);

	//analyze command line input
	CmdInputFactor::getInstance()->analyze(argc, argv);

	//now begin to analyze the files input from command line
	string toCompileFile = CmdInputFactor::getInstance()->getNextFile();
  Lex lex;
  lex.analyzeAFile(toCompileFile);
  LexRecList recList = lex.getRecList();
  ASSERT_EQ(15, recList.size());
  ASSERT_STREQ("main", recList[0].word.c_str());
  ASSERT_STREQ("(", recList[1].word.c_str());
  ASSERT_STREQ("1", recList[2].word.c_str());
  ASSERT_STREQ(",", recList[3].word.c_str());
  ASSERT_STREQ("(", recList[4].word.c_str());
  ASSERT_STREQ("3", recList[5].word.c_str());
  ASSERT_STREQ("+", recList[6].word.c_str());
  ASSERT_STREQ("1", recList[7].word.c_str());
  ASSERT_STREQ(")", recList[8].word.c_str());
  ASSERT_STREQ(",", recList[9].word.c_str());
  ASSERT_STREQ("(", recList[10].word.c_str());
  ASSERT_STREQ("abce", recList[11].word.c_str());
  ASSERT_STREQ(")", recList[12].word.c_str());
  ASSERT_STREQ(")", recList[13].word.c_str());
  ASSERT_STREQ(";", recList[14].word.c_str());
}

TEST(Lex, macroTest3){
  //this is really a strong test, so don't do it for more than one file every time
  int argc = 2;
  char argv0[128] = {0},argv1[128] = {0};
  strcpy(argv0,"tester");
  strcpy(argv1,"./test/TestSet/macro_test_3");
  char* argv[2] = {argv0,argv1};

	JZSetLoggerLevel(JZ_LOG_TEST);

	//analyze command line input
	CmdInputFactor::getInstance()->analyze(argc, argv);

	//now begin to analyze the files input from command line
	string toCompileFile = CmdInputFactor::getInstance()->getNextFile();
  JZWRITE_TEST("toCompileFile:%s", toCompileFile.c_str() );
  Lex lex;
  lex.analyzeAFile(toCompileFile);
  LexRecList recList = lex.getRecList();
  ASSERT_EQ(0, recList.size());
}
