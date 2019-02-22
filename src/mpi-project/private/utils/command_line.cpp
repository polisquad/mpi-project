#include "utils/command_line.h"

#include <unistd.h>

using PairT = std::pair<std::string, std::string>;

CommandLine::CommandLine(int32 argc, char ** argv)
{
	// Flag to check if input was found
	bool bFoundInput = false, bFoundOutput = false;

	// Param we are holding on to
	uint32 currentArg = 0;

	for (uint32 i = 1; i < argc; ++i)
	{
		const char * arg = argv[i];

		if (arg[0] == '-' & arg[1] == '-')
			currentArg = i;
		else if (currentArg != 0)
			valueMap.insert(PairT(argv[currentArg] + 2, arg)), currentArg = 0;
		else if (!bFoundInput)
			valueMap.insert(PairT("input", arg)), bFoundInput = true;
		else if (!bFoundOutput)
			valueMap.insert(PairT("output", arg)), bFoundOutput = true;
	}
}