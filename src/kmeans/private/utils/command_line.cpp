#include "utils/command_line.h"

#include <unistd.h>

CommandLine * CommandLine::singleton = nullptr;

CommandLine::CommandLine(int32 argc, char ** argv)
{
	// Cannot construct two
	assert(!singleton);
	singleton = this;

	for (uint32 i = 1; i < argc; ++i)
	{
		const std::string arg = argv[i];
		if (arg[0] == '-')
		{
			if (
				  arg == "--init"
				| arg == "--mpi-mode"
				| arg == "--clusters"
				| arg == "--max-iter"
				| arg == "--convergence-factor"
			)
			{
				// Arguments that need a value
				if (++i < argc)
					args[arg] = argv[i];
			}
			else
				args[arg] = "";
		}
		else if (access(arg.c_str(), F_OK) != -1)
		{
			// Input or output
			auto inputIt = args.find("input");
			if (inputIt == args.end())
				args.insert(std::make_pair("input", arg));
			else
				args.insert(std::make_pair("output", arg));
		}
		else
		{
			// output?
			auto outputIt = args.find("output");
			if (outputIt == args.end())
				args.insert(std::make_pair("input", arg));
		}
	}
}

CommandLine & CommandLine::get()
{
	return *singleton;
}