#pragma once

#include "core_types.h"
#include "templates/singleton.h"

#include <string>
#include <unordered_map>

/**
 * @class CommandLine utils/command_line.h
 * 
 * A command line parser
 */
class CommandLine : public Singleton<CommandLine>
{
protected:
	/// Parameter values
	std::unordered_map<std::string, std::string> valueMap;

public:
	/// Default constructor
	CommandLine(int32 argc, char ** argv);

	/**
	 * Retrieves parameter value
	 * 
	 * @param [in] name
	 * @param [out] value
	 * @returns true if value exists, false otherwise
	 * @{
	 */
	template<typename T>
	inline bool getValue(const std::string & name, T & value)
	{
		auto it = valueMap.find(name);
		if (it != valueMap.end())
		{
			value = T(it->second);
			return true;
		}

		return false;
	}

	FORCE_INLINE bool getValue(const std::string & name)
	{
		return valueMap.find(name) != valueMap.end();
	}
	/// @}
};

/// @ref getValue() specializations
/// @{
template<>
inline bool CommandLine::getValue(const std::string & name, std::string & value)
{
	auto it = valueMap.find(name);
	if (it != valueMap.end())
	{
		value = it->second;
		return true;
	}

	return false;
}

template<>
inline bool CommandLine::getValue(const std::string & name, int32 & value)
{
	auto it = valueMap.find(name);
	if (it != valueMap.end())
	{
		value = std::stoi(it->second);
		return true;
	}

	return false;
}

template<>
FORCE_INLINE bool CommandLine::getValue(const std::string & name, uint32 & value)
{
	return getValue(name, (int32&)value);
}

template<>
inline bool CommandLine::getValue(const std::string & name, float32 & value)
{
	auto it = valueMap.find(name);
	if (it != valueMap.end())
	{
		value = std::stof(it->second);
		return true;
	}

	return false;
}
/// @}