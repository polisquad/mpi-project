#pragma once

#include "coremin.h"

#include <unordered_map>
#include <string>

class CommandLine
{
protected:
	/// Singleton instance
	static CommandLine * singleton;

	/// Map of command line arguments
	std::unordered_map<std::string, std::string> args;

public:
	/// Default constructor
	CommandLine(int32 argc, char ** argv);

	/// Get argument value
	/// @{
	template<typename T = std::string>
	FORCE_INLINE bool getValue(const std::string & arg, T & val) const
	{
		auto argIt = args.find(arg);
		if (argIt != args.end())
		{
			val = T((*argIt).second);
			return true;
		}

		return false;
	}
	
	FORCE_INLINE bool getValue(const std::string & arg) const
	{
		auto argIt = args.find(arg);
		return argIt != args.end();
	}
	/// @}

	/// Get singleton
	static CommandLine & get();
};

template<>
FORCE_INLINE bool CommandLine::getValue(const std::string & arg, std::string & val) const
{
	auto argIt = args.find(arg);
	if (argIt != args.end())
	{
		val = (*argIt).second;
		return true;
	}

	return false;
}

template<>
FORCE_INLINE bool CommandLine::getValue(const std::string & arg, int32 & val) const
{
	auto argIt = args.find(arg);
	if (argIt != args.end())
	{
		val = std::stoi((*argIt).second);
		return true;
	}

	return false;
}

template<>
FORCE_INLINE bool CommandLine::getValue(const std::string & arg, uint32 & val) const
{
	auto argIt = args.find(arg);
	if (argIt != args.end())
	{
		val = std::stoi((*argIt).second);
		return true;
	}

	return false;
}

template<>
FORCE_INLINE bool CommandLine::getValue(const std::string & arg, float32 & val) const
{
	auto argIt = args.find(arg);
	if (argIt != args.end())
	{
		val = std::stof((*argIt).second);
		return true;
	}

	return false;
}