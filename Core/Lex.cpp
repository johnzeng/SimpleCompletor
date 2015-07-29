#include "Lex.h"
#include "DefineManager.h"
#include "JZFileUtil.h"
#include "JZLogger.h"
#include "JZMacroFunc.h"
#include "IncludeHandler.h"
#include <stdlib.h>
/*********************************************************
	Lex begin here 
 ********************************************************/

Lex::Lex()
{
}

Lex::~Lex()
{
}

uint32 Lex::analyzeAFile(const string& fileName)
{
	JZWRITE_DEBUG("now analyze file : %s", fileName.c_str());
	uint64 bufSize;
	unsigned char* buff = JZGetFileData(fileName.c_str(), &bufSize);
	const char* buffWithOutBackSlant = LexUtil::eraseLineSeperator((const char*)buff,&bufSize);

	JZSAFE_DELETE(buff);
	FileReaderRecord fileRecord = initFileRecord(buffWithOutBackSlant,bufSize,fileName,eFileTypeFile);

	mReaderStack.push(fileRecord);
	mPreprocessedFile.insert(fileName);
	uint32 ret = doLex();
	JZWRITE_DEBUG("analyze file %s end", fileName.c_str());
	return ret;
}

uint32 Lex::doLex()
{
	uint32 ret;
	char seperator;
	string word;
	while(eLexNoError == (consumeWord(word, seperator)))
	{
		if ("" != word)
		{
			if (mReaderStack.top().recordType== eFileTypeMacro &&
				"defined" == word)
			{
				uint32 err = handleIsDefined(word, seperator);
				if (err != eLexNoError)
				{
					writeError(err);
					return err;
				}
				saveWord(word);
			}
			else if (DefineManager::eDefMgrDefined == mDefMgr.isDefined(word))
			{
				uint32 err = handleDefinedWord(word,seperator);
				if (err != eLexNoError)
				{
					writeError(err);
					return err;
				}
			}
			else
			{
				saveWord(word);
			}
		}
		LexPatternHandler handler = LexPtnTbl->getPattern(seperator);
		if (NULL == handler)
		{
			//no handler is registered,means this is a normal input
			if (true == LexUtil::isEmptyInput(seperator))
			{
				continue;
			}
			string toSaveSeperator = "";
			toSaveSeperator += seperator;
			saveWord(toSaveSeperator);
		}
		else
		{
			uint32 err = eLexNoError;
			err = (this->*handler)();
			if (err != eLexNoError)
			{
				writeError(err);
				return err;
			}
		}
	}
	return eLexNoError;
}

uint32 Lex::consumeWord(
		string &retStr,char &retSeperator,
		LexInput skipEmptyInput,
		LexInput inOneLine)
{
	JZFUNC_BEGIN_LOG();
	uint32 ret;
	retStr = "";
	retSeperator = 0;
	while (eLexNoError == (ret = consumeChar(&retSeperator)))
	{
		if (false == LexUtil::isInterpunction(retSeperator))
		{
			retStr += retSeperator;
		}
		else
		{
			if (
				eLexInOneLine == inOneLine &&
				true == LexUtil::isLineEnder(retSeperator)
				)
			{
				JZFUNC_END_LOG();
				return eLexReachLineEnd;
			}
			if (eLexSkipEmptyInput == skipEmptyInput && 
				true == LexUtil::isEmptyInput(retSeperator) &&
			   	"" == retStr)
			{
				continue;
			}
			JZFUNC_END_LOG();
			return eLexNoError;
		}
	}
	JZFUNC_END_LOG();
	return ret;
}

uint32 Lex::handleDefinedWord(const string& word,char &seperator)
{
	JZFUNC_BEGIN_LOG();
	auto defRec = mDefMgr.findDefineMap(word);
	ParamSiteMap indexMap;
	pushDefParam(defRec, &indexMap);
	if (NULL == defRec)
	{
		JZFUNC_END_LOG();
		return eLexWordIsNotDefined;
	}
	if (true == defRec->isFuncLikeMacro)
	{
		if (false == LexUtil::isEmptyInput(seperator) && '(' != seperator)
		{
			return eLexUnexpectedSeperator;
		}
		//read real param
		turnOnFuncLikeMacroMode();

		if ('(' == seperator)
		{
			//already consumed
			handleLeftBracket();
			seperator = ' ';
		}

		uint32 ret = doLex();
		turnOffFuncLikeMacroMode();
		JZWRITE_DEBUG("now add param list");
		//untile here, param list is all get
		if (eLexNoError != ret && eLexReachFileEnd != ret && eLexParamAnalyzeOVer != ret)
		{
			JZFUNC_END_LOG();
			return ret;
		}
		for(int i = 0; i < defRec->formalParam.size() ; i++ )
		{
			string word = defRec->formalParam[i].word;	
			indexMap[word] = i;
		}
		JZWRITE_DEBUG("now print param list");
#ifdef DEBUG
		for(int i = 0 ; i < mRealParamListStack.top().size(); i++)
		{
			for(int j = 0 ; j < mRealParamListStack.top().at(i).size() ;j ++)
			{
				JZWRITE_DEBUG("i is :%d,j is %d,word is :[%s]",i,j,mRealParamListStack.top()[i][j].word.c_str());	
			}
		}
#endif
	}

	//not func like ,just expend it
	char* buff = (char*)malloc(defRec->defineStr.size());
	FileReaderRecord rec = initFileRecord(buff,defRec->defineStr.size(), word,eFileTypeDefine);
	mReaderStack.push(rec);
	uint32 ret = doLex();
	popDefParam();

	if (ret != eLexNoError && ret != eLexReachFileEnd)
	{
		//error happen 
		JZFUNC_END_LOG();
		return ret;
	}
	JZFUNC_END_LOG();
	return eLexNoError;	
	
}
void Lex::pushDefParam(const DefineRec *rec, ParamSiteMap *indexMap)
{
	RealParamList list;
	mRealParamListStack.push(list);
	mDefinePtrStack.push(rec);
	mParamSiteMap.push(indexMap);
}

void Lex::popDefParam()
{
	mRealParamListStack.pop();
	mDefinePtrStack.pop();
	mParamSiteMap.pop();

}

void Lex::writeError(uint32 err)
{
	JZWRITE_DEBUG("err id : %d",err);
	switch(err)
	{
		case eLexReachFileEnd:
		{
			JZWRITE_DEBUG("reach file end");
			break;	
		}
		case eLexReaderStackEmpty:
		{
			JZWRITE_DEBUG("stack end");
			break;	
		}
		default:
		{
			break;	
		}
	}
}

void Lex::saveWordTo(const string& input,LexRecList& list, uint32 recordType)
{
	if (false == isLastStreamUseful())
	{
		//yeah! this stream is not useful!
		return;
	}
	if (true == LexUtil::isEmptyInput(input))
	{
		//don't add empty input
		return;
	}
	LexRec rec = 
	{
		.word = input,
		.line = mReaderStack.top().curLineNum,
		.file = mReaderStack.top().fileName,
		.type = recordType,
		.fileType = mReaderStack.top().recordType,
	};
	list.push_back(rec);
}

void Lex::saveWord(const string& input, uint32 recordType)
{
	saveWordTo(input,mLexRecList, recordType);
}

void Lex::printLexRec()
{
	auto it = mLexRecList.begin();
	for(; it != mLexRecList.end(); it++)
	{
		JZWRITE_DEBUG("line:%d,word:%s",it->line, it->word.c_str());
	}
}

uint32 Lex::undoConsume()
{
	if (mReaderStack.empty())
	{
		return eLexReaderStackEmpty;
	}
	FileReaderRecord &record = mReaderStack.top();
	if (record.curIndex <= 0)
	{
		return eLexAlreadyLastWord;
	}
	record.curIndex--;
	char curChar = record.buffer[record.curIndex];
	if(true == LexUtil::isLineEnder(curChar))
	{
		record.curLineNum --;	
	}
	return eLexNoError;
}

uint32 Lex::consumeChar(char *ret)
{
	*ret = 0;
	if (mReaderStack.empty())
	{
		return eLexReaderStackEmpty;
	}
	FileReaderRecord &record = mReaderStack.top();
	if (record.bufferSize == record.curIndex)
	{
		JZSAFE_DELETE(mReaderStack.top().buffer);
		mReaderStack.pop();
		return eLexReachFileEnd;
	}

	*ret = record.buffer[record.curIndex];
	if (true == LexUtil::isLineEnder(*ret))
	{
		record.curLineNum++;
	}
	record.curIndex++;

	return eLexNoError;
}

/*********************************************************
	when handling ' and ", I simply ignore the \ problem
    I just find the next quotation,so some error will not
	happen in my compilor	
 ********************************************************/

uint32 Lex::handleSingleQuotation()
{
	JZFUNC_BEGIN_LOG();

	//when you enter this func,you have already get a ',so
	string toSave = "'";
	uint32 toSaveLen = 0;
	do
	{
		string retStr = "";
		uint32 retErr = eLexNoError;
		
		retErr = consumeCharUntilReach('\'',&retStr);
		if (eLexNoError != retErr)
		{
			JZFUNC_END_LOG();
			return retErr;
		}
		JZWRITE_DEBUG("ret str is : %s",retStr.c_str());
		toSave += retStr;
		toSaveLen = toSave.size();
	}while(toSaveLen > 2 && true == LexUtil::isBackSlant(toSave[toSaveLen - 2]));

//	handle const char input
//	...
	saveWord(toSave, eLexRecTypeConstChar);

	JZFUNC_END_LOG();
	return eLexNoError;
}

uint32 Lex::handleDoubleQuotation()
{
	JZFUNC_BEGIN_LOG();

	//when you enter this func,you have already get a ",so init a " here
	string toSave = "\"";
	uint32 toSaveLen = 0;
	do
	{
		string retStr = "";
		uint32 retErr = eLexNoError;
		retErr = consumeCharUntilReach('"',&retStr);
		if (eLexNoError != retErr)
		{
			JZFUNC_END_LOG();
			return retErr;
		}
		toSave += retStr;
		toSaveLen = toSave.size();
	}while(toSaveLen > 2 && true == LexUtil::isBackSlant(toSave[toSaveLen - 2]));

//	handle const char input
//	...
	saveWord(toSave, eLexRecTypeConstChar);

	JZFUNC_END_LOG();
	return eLexNoError;
	return 0;
}

uint32 Lex::readChar(char *ret)
{
	*ret = 0;
	if (mReaderStack.empty())
	{
		return eLexReaderStackEmpty;
	}
	FileReaderRecord &record = mReaderStack.top();
	if (record.bufferSize == record.curIndex)
	{
		JZFUNC_END_LOG();
		return eLexReachFileEnd;
	}

	*ret = record.buffer[record.curIndex];

	return eLexNoError;

}


uint32 Lex::handleBar()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "|";
	if (
		'=' == nextChar ||
		'|' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleLeftSharpBracket()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "<";
	if (
		'=' == nextChar ||
		'<' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	//if any error happen,return now
	if (ret != eLexNoError)
	{
		return ret;
	}

	if (0 == toSave.compare("<<") )
	{
		//toSave == "<<"
		char nextChar = 0;
		ret = readChar(&nextChar);
		if ('=' == nextChar)
		{
			toSave += nextChar;
		}
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleComma()
{
	if (true == isFuncLikeMacroMode())
	{
		if (mBracketMarkStack.empty())
		{
			return eLexUnknowError;
		}
		JZWRITE_DEBUG("now handle comma, brack stack size:%ld",mBracketMarkStack.top().size());
		if (1 == mBracketMarkStack.top().size())
		{
			JZWRITE_DEBUG("save a Param ");
			//save it!
			uint32 beginMark = mBracketMarkStack.top().top();
			LexRecList list;
			list.resize(mLexRecList.size() - beginMark);
			for(int i = mLexRecList.size() - 1 ; i >= beginMark; i--)
			{
				list[i - beginMark] = mLexRecList[i];
				mLexRecList.pop_back();
			}
			if (mRealParamListStack.empty())
			{
				return eLexUnknowError;
			}
			mRealParamListStack.top().push_back(list);
			return eLexNoError;
		}
	}
	saveWord(",");
	return eLexNoError;
}

uint32 Lex::handleRightSharpBracket()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = ">";
	if (
		'=' == nextChar ||
		'>' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	//if any error happen,return now
	if (ret != eLexNoError)
	{
		return ret;
	}

	if (0 == toSave.compare(">>") )
	{
		//toSave == ">>"
		nextChar = 0;
		ret = readChar(&nextChar);
		if ('=' == nextChar)
		{
			consumeChar(&nextChar);
			toSave += nextChar;
		}
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleLeftBracket()
{
	JZFUNC_BEGIN_LOG();
	if (true == isFuncLikeMacroMode())
	{
		//now it is analyzing macro like func,do something
		JZWRITE_DEBUG("now push a mark");
		if (mBracketMarkStack.empty())
		{
			return eLexUnknowError;
		}
		uint32 mark = mLexRecList.size();
		mBracketMarkStack.top().push(mark);
		JZWRITE_DEBUG("push over");
		if (1 == mBracketMarkStack.top().size())
		{
			JZWRITE_DEBUG("first bracket,don't save");
			return eLexNoError;
		}
	}
	saveWord("(");
	JZFUNC_END_LOG();
	return eLexNoError;
}

uint32 Lex::handleRightBracket()
{
	JZFUNC_BEGIN_LOG();
	if (true == isFuncLikeMacroMode())
	{
		JZWRITE_DEBUG("now pop a mark");
		if (mBracketMarkStack.empty())
		{
			return eLexUnknowError;
		}
		if (mBracketMarkStack.top().empty())
		{
			return eLexUnknowError;
		}
		if (1 == mBracketMarkStack.top().size())
		{
			JZWRITE_DEBUG("save a Param ");
			//save it!
			uint32 beginMark = mBracketMarkStack.top().top();
			LexRecList list;
			list.resize(mLexRecList.size() - beginMark);
			for(int i = mLexRecList.size() - 1 ; i >= beginMark; i--)
			{
				list[i - beginMark] = mLexRecList[i];
				mLexRecList.pop_back();
			}
			if (mRealParamListStack.empty())
			{
				return eLexUnknowError;
			}
			if (list.size() > 0)
			{
				mRealParamListStack.top().push_back(list);
			}
		}
		mBracketMarkStack.top().pop();
		if (mBracketMarkStack.top().empty())
		{
			return eLexParamAnalyzeOVer;
		}

	}
	saveWord(")");
	JZFUNC_END_LOG();
	return eLexNoError;
}

uint32 Lex::handlePoint()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = ".";
	if ('.' == nextChar)
	{
		nextChar = 0;
		ret = readChar(&nextChar);
		if ('.' == nextChar)
		{
			//consume two char:".."
			consumeChar(&nextChar);
			consumeChar(&nextChar);
			toSave = "...";
		}
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleAnd()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "&";
	if (
		'=' == nextChar ||
		'&' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleEqual()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "=";
	if ('=' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleMinus()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "-";
	if (
		'-' == nextChar||	//--
		'>' == nextChar||	//->
		'=' == nextChar)	//-=
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleWave()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "~";
	if ('=' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleUpponSharp()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "^";
	if ('=' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handlePlus()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "+";
	if (
		'+' == nextChar||
		'=' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleStart()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "*";
	if ('=' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleExclamation()
{
	//handle "!"
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "!";
	if ('=' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleDivideSlant()
{
	uint32 ret = eLexNoError;
	char nextChar = 0;
	ret = readChar(&nextChar);
	string toSave = "&";
	if ('=' == nextChar)
	{
		consumeChar(&nextChar);
		toSave += nextChar;
	}
	saveWord(toSave);
	return ret;
}

uint32 Lex::handleSlant()
{
	JZFUNC_BEGIN_LOG();
	char nextChar = 0;
	uint32 ret = eLexNoError;
	ret = readChar(&nextChar);
	switch(nextChar)
	{
		case '/':
		{
			ret = readChar(&nextChar);
			JZFUNC_END_LOG();
			return handleCommentLine();	
		}
		case '*':
		{
			ret = readChar(&nextChar);
			JZFUNC_END_LOG();
			return handleCommentBlock();	
		}
		default:
		{
			//maybe this means divide
			JZFUNC_END_LOG();
			return handleDivideSlant();
		}
	}

	JZFUNC_END_LOG();
	return eLexNoError;
}

uint32 Lex::handleSharpPragma()
{
	uint32 ret = eLexNoError;
	string word;
	char seperator;
	ret = consumeWord(word,seperator,eLexSkipEmptyInput,eLexInOneLine);
	//there is other more word to handle,but I just care about once
	//if more key word is to handle,I will make a func ptr map table
	if (word == "once")
	{
		mOnceFileSet.insert(mReaderStack.top().fileName);
	}
	if (false == LexUtil::isLineEnder(seperator))
	{
		ret = consumeCharUntilReach('\n',&word);
	}
	return ret ;
}

uint32 Lex::handleCommentLine()
{
	JZFUNC_BEGIN_LOG();
	char nextChar = 0;
	uint32 ret = eLexNoError;
	bool backSlantBeforeLineSeperator = false;

	while ((ret = consumeChar(&nextChar)) == eLexNoError)
	{
		if (false == LexUtil::isEmptyInput(nextChar) && true == backSlantBeforeLineSeperator)
		{
			backSlantBeforeLineSeperator = false;
			continue;
		}
		if ('\\' == nextChar)
		{
			backSlantBeforeLineSeperator = true;
			continue;
			
		}
		if (true == LexUtil::isLineEnder(nextChar))
		{
			if (true == backSlantBeforeLineSeperator)
			{

				JZWRITE_DEBUG("line ended with back slant");
				continue;
			}
			else
			{
				break;
			}
		}
	}
	JZFUNC_END_LOG();
	return ret;
}

uint32 Lex::handleCommentBlock()
{
	JZFUNC_BEGIN_LOG();
	uint32 ret = eLexNoError;
	string retStr = "";
	char retChar = 0;

	do
	{
		ret = consumeCharUntilReach('*', &retStr);
		if (eLexNoError == ret)
		{
			ret = readChar(&retChar);
		}
		else
		{
			return ret;
		}
		if (retChar == '/')
		{
			char slant;
			consumeChar(&slant);
			break;
		}
	}while(ret == eLexNoError);

	JZFUNC_END_LOG();
	return ret;
}

uint32 Lex::handleSharp()
{
	JZFUNC_BEGIN_LOG();

	uint32 ret = eLexNoError;
	char nextChar = 0;
//try if this is double sharp: ##
	ret = readChar(&nextChar);
	if (nextChar == '#')
	{
		consumeChar(&nextChar);
		saveWord("##");
		JZFUNC_END_LOG();
		return eLexNoError;
	}


	string word = "";

	ret = consumeWord(word, nextChar, eLexSkipEmptyInput, eLexInOneLine);
	//in this situation, reach line end means error
	if (ret != eLexNoError)
	{
		if (ret == eLexReachLineEnd && false == LexUtil::isEmptyInput(word))
		{
			JZWRITE_DEBUG("simply reach line end");	
		}
		else
		{
			JZFUNC_END_LOG();
			return ret;
		}
	}
	if (false == LexUtil::isEmptyInput(nextChar))
	{
		JZFUNC_END_LOG();
		return eLexUnknowMacro;
	}

	//pop the seperator out,in case if the seperator is \n,
	//I check it in the handles,so don't eat it
	undoConsume();

	LexPatternHandler handler = LexPtnTbl->getMacroPattern(word);
	if (NULL == handler)
	{
		JZWRITE_DEBUG("no handler ,maybe this is a sharp input");
		saveWord("#");
	}
	else
	{
		uint32 err = eLexNoError;
		err = (this->*handler)();
		if (err != eLexNoError)
		{
			JZFUNC_END_LOG();
			return err;
		}
	}
	JZFUNC_END_LOG();
	return eLexNoError;
}

uint32 Lex::handleSharpElse()
{
	JZFUNC_BEGIN_LOG();
	string word = "";
	char seperator;
	uint32 ret = consumeWord(
			word,seperator,eLexSkipEmptyInput,eLexInOneLine);
	if (eLexNoError != ret && ret != eLexReachLineEnd)
	{
		JZFUNC_END_LOG();
		return ret;
	}
	if (false ==  LexUtil::isEmptyInput(word))
	{
		JZWRITE_DEBUG("word is :[%s]",word.c_str());
		JZFUNC_END_LOG();
		return eLexSharpElseFollowWithOtherThing;
	}
	JZFUNC_END_LOG();
	return pushPrecompileStreamControlWord(eLexPSELSE);

}

uint32 Lex::handleSharpEndIf()
{
	JZFUNC_BEGIN_LOG();
	string word = "";
	char seperator;
	uint32 ret = consumeWord(
			word,seperator,eLexSkipEmptyInput,eLexInOneLine);
	if (eLexNoError != ret && ret != eLexReachLineEnd)
	{
		JZFUNC_END_LOG();
		return ret;
	}
	if (false ==  LexUtil::isEmptyInput(word))
	{
		JZFUNC_END_LOG();
		return eLexSharpEndIfFollowWithOtherThing;
	}
	JZFUNC_END_LOG();
	return pushPrecompileStreamControlWord(eLexPSENDIF);

}
uint32 Lex::handleSharpIfndef()
{
	JZFUNC_BEGIN_LOG();
	uint32 ret;
	char seperator;
	string word;
	ret = consumeWord(word,seperator,eLexSkipEmptyInput, eLexInOneLine);
	if (eLexNoError != ret)
	{
		JZFUNC_END_LOG();
		return ret;
	}
	JZWRITE_DEBUG("word is :[%s]",word.c_str());
	//check if word is defined
	if (true == LexUtil::isEmptyInput(word))
	{
		JZFUNC_END_LOG();
		return eLexSharpIfdefFollowedWithNothing;
	}
	
	bool isDef = (DefineManager::eDefMgrDefined == mDefMgr.isDefined(word));
	JZWRITE_DEBUG("word:%s,is define :%d",word.c_str(), isDef);

	ret = pushPrecompileStreamControlWord(eLexPSIFNDEF, !isDef);
	
	JZFUNC_END_LOG();
	return ret ;
}

uint32 Lex::handleSharpIfdef()
{
	JZFUNC_BEGIN_LOG();
	uint32 ret;
	char seperator;
	string word;
	ret = consumeWord(word,seperator,eLexSkipEmptyInput, eLexInOneLine);
	if (eLexNoError != ret)
	{
		JZFUNC_END_LOG();
		return ret;
	}
	JZWRITE_DEBUG("word is :[%s]",word.c_str());
	//check if word is defined
	if (true == LexUtil::isEmptyInput(word))
	{
		JZFUNC_END_LOG();
		return eLexSharpIfdefFollowedWithNothing;
	}
	
	bool isDef = (DefineManager::eDefMgrDefined == mDefMgr.isDefined(word));
	JZWRITE_DEBUG("word:%s,is define :%d",word.c_str(), isDef);

	ret = pushPrecompileStreamControlWord(eLexPSIFDEF, isDef);
	
	JZFUNC_END_LOG();
	return ret ;
}

uint32 Lex::handleIsDefined(string& ret, char &seperator)
{
	JZFUNC_BEGIN_LOG();
	string word;
	uint32 errRet;
	if (seperator != '(' && false == LexUtil::isEmptyInput(seperator))
	{
		JZFUNC_END_LOG();
		return eLexUnexpectedSeperator;
	}
	if (seperator != '(')
	{
		errRet = consumeCharUntilReach('(',&word,eLexInOneLine);
		if (errRet != eLexNoError)
		{
			JZFUNC_END_LOG();
			return errRet;
		}
	}
	else
	{
		seperator = ' ';	
	}
	char nextChar;
	errRet = consumeWord(word, nextChar, eLexSkipEmptyInput, eLexInOneLine);
	if (errRet != eLexNoError)
	{
		JZFUNC_END_LOG();
		return errRet;
	}
	if (nextChar != ')')
	{
		JZFUNC_END_LOG();
		return eLexUnexpectedSeperator;
	}

	if (DefineManager::eDefMgrDefined == mDefMgr.isDefined(word))
	{
		ret = "1";
	}
	else
	{
		ret = "0";	
	}
	JZWRITE_DEBUG("ret word is :%s",ret.c_str());
	JZFUNC_END_LOG();
	return errRet;
}

uint32 Lex::pushPrecompileStreamControlWord(uint32 mark,bool isSuccess)
{
	JZFUNC_BEGIN_LOG();
	PrecompileSelector ps = 
	{
		.mark = mark,
		.isSuccess = isSuccess,
		.tag = mPSStack.size() + 1,
	};

	//set begin tag
	switch(mark)
	{
		case eLexPSIF:
		case eLexPSIFNDEF:
		case eLexPSIFDEF:
			ps.beginTag = ps.tag;
			break;
		default:
		{
			if (0 == mPSStack.size())
			{
				JZWRITE_DEBUG("mark is :%d",mark);
				return eLexUnmatchMacro;
			}
			ps.beginTag = mPSStack.back().beginTag;
		}
	};

	//check if this stream is really success
	switch(mark)
	{
		case eLexPSIF:
		case eLexPSIFNDEF:
		case eLexPSIFDEF:
			//do nothing
			break;
		case eLexPSELSE:
		case eLexPSELIF:
		{
			if (false == ps.isSuccess)
			{
				break;
			}
			for(int i = ps.beginTag - 1; i < mPSStack.size(); i++)
			{
				if (true == mPSStack[i].isSuccess)
				{
					ps.isSuccess =false;
					break;
				}
			}
			uint32 offStream = getCompileStream();
			if (0 != offStream)
			{
				if (ps.beginTag == mPSStack[offStream - 1].beginTag)
				{
					turnOnCompileStream();
				}
			}
			break;
		}
		case eLexPSENDIF:
		{
			uint32 offStream = getCompileStream();
			if (0 != offStream)
			{
				if (ps.beginTag == mPSStack[offStream - 1].beginTag)
				{
					turnOnCompileStream();
				}
			}
			while (false == mPSStack.empty() && ps.beginTag == mPSStack.back().beginTag)
			{
				mPSStack.pop_back();
			}
			return eLexNoError;
		}
		default:
		{
			JZWRITE_DEBUG("unregister mark:%d", mark);	
			return eLexUnknowMacro;
		}
	}

	if (true == isLastStreamUseful() && false == ps.isSuccess)
	{
		turnOffCompileStream(ps.tag);
	}

	mPSStack.push_back(ps);
	JZFUNC_END_LOG();
	return eLexNoError;
}

void Lex::turnOnCompileStream()
{
	if (!mReaderStack.empty())
	{
		mReaderStack.top().mStreamOffTag = 0;
	}
}

uint32 Lex::getCompileStream()
{
	if (mReaderStack.empty())
	{
		return 0;
	}
	return mReaderStack.top().mStreamOffTag;
}

void Lex::turnOffCompileStream(uint32 tag)
{
	if (!mReaderStack.empty())
	{
		mReaderStack.top().mStreamOffTag = tag;
	}
}

bool Lex::isLastMacroSuccess()
{
	if (true == mPSStack.empty())
	{
		//if stack is empty ,always return true;
		return true;
	}
	return mPSStack.back().isSuccess;
}

bool Lex::isLastStreamUseful()
{
	return (getCompileStream() == 0) && isLastMacroSuccess();
}

uint32 Lex::handleSharpDefine()
{
	JZFUNC_BEGIN_LOG();

	/*********************************************************
		now handle define! 
	 ********************************************************/
	
	uint32 ret = eLexNoError;
	char seperator = 0;
	string key = "";

	ret = consumeWord(key, seperator);
	if (eLexNoError != ret)
	{
		return ret;
	}

	DefineRec defineRec;
	defineRec.key = key;
	if ('(' == seperator)
	{
		defineRec.isFuncLikeMacro = true;
		//read format parm
		string param;
		char seperator;
		uint32 paramRet;
		while(eLexNoError == (paramRet = consumeWord(param,seperator)))
		{
			if (param != "" && (seperator == ')' || seperator == ','))
			{
				JZWRITE_DEBUG("save a param :%s",param.c_str());
				if (defineRec.formalParam.size() > 0 &&
				    defineRec.formalParam.back().type == eLexRecTypeFuncLikeMacroVarParam )
				{
					//should not save more param
					JZWRITE_DEBUG("val param end with not val param:%s",param.c_str());
					return eLexValParamNotLast;
				}
				saveWordTo(param ,defineRec.formalParam, eLexRecTypeFuncLikeMacroParam);
			}
			if (seperator == '.')
			{
				if (eLexNoError == tryToMatchWord(".."))
				{
					defineRec.isVarArgs = true;
					if ("" != param)
					{
						saveWordTo(param,defineRec.formalParam,eLexRecTypeFuncLikeMacroVarParam);
					}
				}
				else
				{
					JZWRITE_DEBUG("reach here");
					return eLexUnexpectedSeperator;
				}
				continue;
			}
			if (seperator == ')')
			{
				//reach end
				break;
			}
			if (seperator != ',')
			{
				JZWRITE_DEBUG("reach here,seperator is : %c",seperator);
				return eLexUnexpectedSeperator;			
			}
		}
	}
	else
	{
		defineRec.isVarArgs = false;
		defineRec.isFuncLikeMacro =false;
	}

	string defWord = "";
	uint32 retErr = eLexNoError;
	
	retErr = consumeCharUntilReach('\n',&defWord);
	if (eLexNoError != retErr)
	{
		JZFUNC_END_LOG();
		return retErr;
	}

	if (defWord.back() == '\n')
	{
		defWord = defWord.substr(0,defWord.size() - 1);
	}

	defineRec.defineStr = defWord;
	mDefMgr.addDefineMap(key, defineRec);	
	JZFUNC_END_LOG();
	return ret;
}

uint32 Lex::checkMacro(bool *isSuccess)
{
	string logicStr = "";
	uint32 retErr = eLexNoError;
	
	retErr = consumeCharUntilReach('\n',&logicStr);
	if (eLexNoError != retErr)
	{
		return retErr;
	}
	char* buff = (char*)malloc(logicStr.size());
	if (NULL == buff)
	{
		return eLexUnknowError;
	}
	memcpy(buff,logicStr.c_str(), logicStr.size());
	FileReaderRecord record = initFileRecord(buff,logicStr.size(),"",eFileTypeMacro);
	mReaderStack.push(record);

	//pop until reach last
	int lastRecPtr = mLexRecList.size();
	uint32 lexret = doLex();
	if (lexret != eLexNoError && lexret != eLexReachFileEnd)
	{
		return lexret;
	}

	LexRecList list;
	list.resize(mLexRecList.size() - lastRecPtr);
	JZWRITE_DEBUG("list size :%ld",list.size());

	for(int i = mLexRecList.size() - 1; i >= lastRecPtr; i--)
	{
		list[i - lastRecPtr] = mLexRecList[i];
		mLexRecList.pop_back();
	}
	JZWRITE_DEBUG("pop fin");
#ifdef DEBUG
	for(int i = 0 ; i < list.size() ; i++)
	{
		JZWRITE_DEBUG("word:[%s]",list[i].word.c_str());	
	}
#endif
	if (NULL == isSuccess)
	{
		//no ptr, this is a comsume input
		return eLexNoError;
	}
	uint32 ret = isMacroSuccess(list, isSuccess);
	return ret;

}
uint32 Lex::handleSharpElif()
{
	bool isSuccess = false;
	uint32 ret = checkMacro(&isSuccess);
	if (ret != eLexNoError)
	{
		return ret;
	}
	return pushPrecompileStreamControlWord(eLexPSELIF, isSuccess);
}

uint32 Lex::handleSharpIf()
{
	bool isSuccess = false;
	uint32 ret = checkMacro(&isSuccess);
	if (ret != eLexNoError)
	{
		return ret;
	}
	return pushPrecompileStreamControlWord(eLexPSIF, isSuccess);
}

uint32 Lex::handleSharpInclude()
{
	uint32 ret = eLexNoError;
	string word;
	char seperator;
	ret = this->consumeWord(word,seperator);
	if (ret != eLexNoError)
	{
		return ret;
	}
	if (false == LexUtil::isEmptyInput(word))
	{
		JZWRITE_DEBUG("include follow with not empty input");
		return eLexUnknowError;
	}
	if (seperator != '<' && seperator != '"')
	{
		JZWRITE_DEBUG("include follow with error seperator");
		return eLexUnexpectedSeperator;
	}

	if (false == isLastStreamUseful())
	{
		//if stream is off, don't do follow analyze
		return eLexNoError;
	}
	string toMatch = "";
	ret = consumeCharUntilReach(LexUtil::seperatorMatcher(seperator),&toMatch);
	if (ret  != eLexNoError)
	{
		JZWRITE_DEBUG("can not match the seperator in #include for:%c", seperator);
		return ret;
	}
	string toIncludeFile = toMatch.substr(0,toMatch.size() - 1); 

//	my test show that clang compiler do not eat empty input..
//	maybe some people will name their file with empty end? I doublt it,
//	but I stick to the clang compiler, so I just skip this one here

//	toIncludeFile = LexUtil::eatLREmptyInput(toIncludeFile);
	JZWRITE_DEBUG("to include file is :[%s]", toIncludeFile.c_str());

	string fullPath = IncludeHandler::getInstance()->getFullPathForIncludeFile(toIncludeFile);

	if (fullPath != "")
	{
		uint32 analyzeRet = this->analyzeAFile(fullPath);
		if (analyzeRet != eLexNoError && analyzeRet != eLexReachFileEnd)
		{
			return analyzeRet;
		}
	}
	else
	{
		JZWRITE_DEBUG("no such file");	
	}
	return eLexNoError;
}

uint32 Lex::tryToMatchWord(const string& word)
{
	JZFUNC_BEGIN_LOG();
	if ("" == word)
	{
		JZFUNC_END_LOG();
		return eLexNoError;
	}
	if (mReaderStack.empty())
	{
		JZFUNC_END_LOG();
		return eLexReaderStackEmpty;
	}
	FileReaderRecord &record = mReaderStack.top();
	if (record.bufferSize <= record.curIndex + word.size())
	{
		JZSAFE_DELETE(mReaderStack.top().buffer);
		mReaderStack.pop();
		JZFUNC_END_LOG();
		return eLexReachFileEnd;
	}

	string subStrWord = "";
	for(int i  = 0 ; i < word.size() ; i ++)
	{
		subStrWord += record.buffer[record.curIndex + i];	
	}
	if (subStrWord == word)
	{
		record.curIndex += word.size();
		JZFUNC_END_LOG();
		return eLexNoError;
	}

	JZFUNC_END_LOG();
	return eLexWordNotMatch;
	
}

uint32 Lex::consumeCharUntilReach(const char inputEnder, string *ret, LexInput inOneLine)
{
	if (NULL == ret)
	{
		return eLexUnknowError;
	}
	*ret = "";
	char nextInput;
	uint32 readRet = eLexNoError;
	while ((readRet = consumeChar(&nextInput)) == eLexNoError)
	{
		*ret += nextInput;
		if (
			eLexInOneLine == inOneLine &&
			true == LexUtil::isLineEnder(nextInput)
			)
		{
			return eLexReachLineEnd;
		}
		if (nextInput == inputEnder)
		{
			break;
		}
	}
	return readRet;
}

bool Lex::isMacroExpending(const string& input)
{
	if (mPreprocessingMacroSet.find(input) == mPreprocessingMacroSet.end())
	{
		return false;
	}
	return true;
}

bool Lex::isOnceFile(const string& input)
{
	if (mOnceFileSet.find(input) == mOnceFileSet.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Lex::turnOnFuncLikeMacroMode()
{
	BracketMarkStack bracketStack;
	
	mFuncLikeMacroParamAnalyzing.push(true);
	mBracketMarkStack.push(bracketStack);
}

void Lex::turnOffFuncLikeMacroMode()
{
	if (false == mFuncLikeMacroParamAnalyzing.empty())
	{
		mFuncLikeMacroParamAnalyzing.pop();
		mBracketMarkStack.pop();
	}
}

bool Lex::isFuncLikeMacroMode()
{
	return !mFuncLikeMacroParamAnalyzing.empty() && !mBracketMarkStack.empty();
}

/*********************************************************
	Lex end here
	now begin the LexUtil   	
 ********************************************************/

bool LexUtil::isInterpunction(const char input)
{
	if (true == isLineEnder(input))
	{
		return true;
	}

	if (true == isEmptyInput(input))
	{
		return true;
	}

	if (true == isBackSlant(input))
	{
		return true;
	}
	switch(input)
	{
		case '|':
		case '^':
		case '=':
		case '+':
		case '/':
		case '*':
		case '-':
		case '~':
		case '\'':
		case '"':
		case ' ':
		case ',':
		case '.':
		case '!':
		case '?':
		case '&':
		case '(':
		case ')':
		case '[':
		case ']':
		case '{':
		case '}':
		case '<':
		case '>':
		case '#':
		case '\t':
		case '\n':
		case ';':
		case ':':
		{
			return true;
		}
		default:
		{
			break;	
		}
	}
	return false;
}

bool LexUtil::isLineEnder(const char input)
{
	if ('\n' == input)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool LexUtil::isEmptyInput(const char input)
{
	switch(input)
	{
		case '\t':
		case '\n':
		case ' ':
			return true;
		default:
		{
			return false;
		}
	}
}

bool  LexUtil::isBackSlant(const char input)
{
	if ('\\' == input)
	{
		return true;
	}
	else
	{
		return false;
	}
}
bool LexUtil::isEmptyInput(const string& input)
{
	int len = input.size();
	for (int i = 0; i < len; i++) {
		if (false == isEmptyInput(input[i]))
		{
			return false;
		}
	}
	return true;	
}

bool LexUtil::isEndWithBackSlant(const string& input)
{
	bool ret = false;
	for(int32 i = input.size() - 1 ; i >= 0 ; i--)
	{
		if (input[i] == '\\')
		{
			ret = true;
			break;
		}
		else if(false == isEmptyInput(input[i]))
		{
			break;
		}
	}
	return ret;
}

char LexUtil::seperatorMatcher(const char input)
{
	char ret = 0;
	switch(input)
	{
		case '"':
			ret = '"';
			break;
		case '\'':
			ret = '\'';
			break;
		case '<':
			ret = '>';
			break;
		case '(':
			ret = ')';
			break;
		case '{':
			ret = '}';
			break;
		default:
		{
			JZWRITE_DEBUG("not registed input : %c",input);	
			break;
		}
	}
	return ret;
}

char* LexUtil::eraseLineSeperator(const char* input,uint64 *bufSize)
{
	JZFUNC_BEGIN_LOG();
	char *ret = (char*)malloc((*bufSize)*sizeof(char));
	uint64 j = 0;
	memset(ret,0,(*bufSize) * sizeof(char));

	JZWRITE_DEBUG("buff size is :%lld",*bufSize);
	for(uint64 i = 0 ; i < (*bufSize); i++)
	{
		if (false == isBackSlant(input[i]))
		{
			ret[j++] = input[i];	
			continue;
		}
		//check next not empty input
		int k = i + 1;
		bool endWithBackSlant = true;
		while(k < (*bufSize) && false == isLineEnder(input[k]))
		{
			if (false == isEmptyInput(input[k]))
			{
				endWithBackSlant = false;
				break;
			}
			k++;
		};
		if (true == endWithBackSlant)
		{
			i = k;
		}
	}
	*bufSize = j;
	JZFUNC_END_LOG();
	return ret;
}

string LexUtil::eatLREmptyInput(const string& toBeEatan)
{
	string ret = "";
	int leftEmptyNum = 0;
	int rightEmptyNum = 0;
	for(int i = 0; i < toBeEatan.size(); i++)
	{
		if (false == isEmptyInput(toBeEatan[i]))
		{
			break;
		}
		leftEmptyNum++;
	}
	for(int i = toBeEatan.size() - 1; i >= 0; i-- )
	{
		if (false == isEmptyInput(toBeEatan[i]))
		{
			break;
		}
		rightEmptyNum++;	
	}
	for(int i  = leftEmptyNum ; i + rightEmptyNum < toBeEatan.size() ; i++)
	{
		ret += toBeEatan[i];	
	}
	return ret;
}

/*********************************************************
	LexUtil End here, now begin the lex pattern table 
 ********************************************************/

LexPatternTable::LexPatternTable()
{
	init();
}

LexPatternTable::~LexPatternTable()
{
	mPatternHandlerMap.clear();
}

LexPatternTable* LexPatternTable::getInstance()
{
	static LexPatternTable* instance = NULL;
	if (NULL == instance)
	{
		instance = new LexPatternTable();
	}
	return instance;
}

void LexPatternTable::init()
{
	/*********************************************************
		init pattern map here 
	 ********************************************************/
	mPatternHandlerMap['\''] = &Lex::handleSingleQuotation;
	mPatternHandlerMap['"']  = &Lex::handleDoubleQuotation;
	mPatternHandlerMap['#']  = &Lex::handleSharp;
	mPatternHandlerMap['/']  = &Lex::handleSlant;
	mPatternHandlerMap['|']  = &Lex::handleBar;
	mPatternHandlerMap['.']  = &Lex::handlePoint;
	mPatternHandlerMap['<']  = &Lex::handleLeftSharpBracket;
	mPatternHandlerMap['>']  = &Lex::handleRightSharpBracket;
	mPatternHandlerMap['&']  = &Lex::handleAnd;
	mPatternHandlerMap['=']  = &Lex::handleEqual;
	mPatternHandlerMap['*']  = &Lex::handleStart;
	mPatternHandlerMap['!']  = &Lex::handleExclamation;
	mPatternHandlerMap['+']  = &Lex::handlePlus;
	mPatternHandlerMap['-']  = &Lex::handleMinus;
	mPatternHandlerMap['^']  = &Lex::handleUpponSharp;
	mPatternHandlerMap['~']  = &Lex::handleWave;
	mPatternHandlerMap['(']  = &Lex::handleLeftBracket;
	mPatternHandlerMap[')']  = &Lex::handleRightBracket;
	mPatternHandlerMap[',']  = &Lex::handleComma;

	/*********************************************************
		init marco pattern map here 
	 ********************************************************/

	mMacroPatternHandlerMap["ifdef"]   = &Lex::handleSharpIfdef;	
	mMacroPatternHandlerMap["ifndef"]   = &Lex::handleSharpIfndef;	
	mMacroPatternHandlerMap["else"]    = &Lex::handleSharpElse;	
	mMacroPatternHandlerMap["if"]      = &Lex::handleSharpIf;	
	mMacroPatternHandlerMap["endif"]   = &Lex::handleSharpEndIf;	
	mMacroPatternHandlerMap["define"]  = &Lex::handleSharpDefine;	
	mMacroPatternHandlerMap["include"] = &Lex::handleSharpInclude;	
	mMacroPatternHandlerMap["pragma"]  = &Lex::handleSharpPragma;
}

LexPatternHandler LexPatternTable::getMacroPattern(const string& input)
{
	if (mMacroPatternHandlerMap.find(input) != mMacroPatternHandlerMap.end())
	{
		return mMacroPatternHandlerMap[input];
	}
	else
	{
		return NULL;	
	}

}

LexPatternHandler LexPatternTable::getPattern(const char input)
{
	if (mPatternHandlerMap.find(input) != mPatternHandlerMap.end())
	{
		return mPatternHandlerMap[input];
	}
	else
	{
		return NULL;	
	}
}
/*********************************************************
	data init helper func 
 ********************************************************/

FileReaderRecord initFileRecord(const char* buff,uint64 size,const string& fileName,uint32 recordType)
{
	FileReaderRecord ret = 
	{
		.buffer = buff,
		.bufferSize = size,
		.curIndex = 0,
		.curLineNum = 1,
		.fileName = fileName,
		.recordType = recordType,
		.mStreamOffTag = 0,	
	};
	return ret;
}

