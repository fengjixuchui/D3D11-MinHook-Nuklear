#pragma once

#include <cstdint>
#include <windows.h>
#include <cstring>
#include <psapi.h>

inline size_t FindPatternDump(const unsigned char* dump, const size_t length, const unsigned char* pattern, const char* mask, size_t& outOffset)
{
	const size_t maskLength = std::strlen(mask) - 1;
	size_t patternPos = 0;
	size_t tmpOffset = -1;
	for (size_t offset = 0; offset < length - 1; ++offset)
	{
		if (dump[offset] == pattern[patternPos] || mask[patternPos] == '?')
		{
			if (mask[patternPos + 1] == '\0')
			{
				outOffset = (offset - maskLength);
				return true;
			}
			if (tmpOffset == -1)
			{
				tmpOffset = offset;
			}
			patternPos++;
		}
		else
		{
			if (tmpOffset > -1)
			{
				offset = tmpOffset;
				tmpOffset = -1;
			}
			patternPos = 0;
		}
	}

	return false;
}

inline uintptr_t FindPattern(const uintptr_t start, const size_t length, const unsigned char* pattern, const char* mask)
{
	size_t pos = 0;
	const auto maskLength = std::strlen(mask) - 1;

	uintptr_t tmpAddress = 0;
	const auto moduleLength = start + length - 1;
	for (auto it = start; it < moduleLength; ++it)
	{
		if (*reinterpret_cast<unsigned char*>(it) == pattern[pos] || mask[pos] == '?')
		{
			if (mask[pos + 1] == '\0')
			{
				return it - maskLength;
			}
			if (tmpAddress == 0)
			{
				tmpAddress = it;
			}
			pos++;
		}
		else
		{
			if (tmpAddress > 0)
			{
				it = tmpAddress;
				tmpAddress = 0;
			}
			pos = 0;
		}
	}

	return 0;
}