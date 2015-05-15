#include "JZLogger.h"
#include "CommandLineFactorMngr.h"

int main()
{
	JZLoggerInit();
	JZSetLoggerLevel(JZ_LOG_ALL);
	JZSetLoggerOutPutStatue(1,0);
	JZWRITE_DEBUG("hello");
	CommandLineFactorMngr::getInstance()->searchAddonFactorFile();
	CommandLineFactorMngr::getInstance()->addonFactorAnalyser();
}
