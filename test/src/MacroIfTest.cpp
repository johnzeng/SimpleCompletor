#include "MacroLex.h"
#include "CmdInputFactor.h"
#include <gtest/gtest.h>
#include "JZLogger.h"

//cover case: basic test
TEST(MacroLex, macroIfTest1){
  //this is really a strong test, so don't do it for more than one file every time
  int argc = 2;
  char argv0[128] = {0},argv1[128] = {0};
  strcpy(argv0,"tester");
  strcpy(argv1,"./test/TestSet/macro_if_test_1");
  char* argv[2] = {argv0,argv1};

	JZSetLoggerLevel(JZ_LOG_TEST);

	//analyze command line input
	CmdInputFactor::getInstance()->analyze(argc, argv);

	//now begin to analyze the files input from command line
	string toCompileFile = CmdInputFactor::getInstance()->getNextFile();

  MacroLex lex;
  lex.analyzeAFile(toCompileFile);
	JZSetLoggerLevel(JZ_LOG_DEBUG);
  lex.printLexRec();
	JZSetLoggerLevel(JZ_LOG_TEST);

  LexRecList recList = lex.getRecList();

  ASSERT_EQ(3, recList.size());
  ASSERT_STREQ("hello", recList[0].word.c_str());
  ASSERT_STREQ("non0", recList[1].word.c_str());
  ASSERT_STREQ("yes", recList[2].word.c_str());
}