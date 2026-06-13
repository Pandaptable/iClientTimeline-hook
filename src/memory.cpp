#include <cctype>
#include <print>
#include <vector>

#ifdef _WIN32
#include <Windows.h>

#include <Psapi.h>
#else
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

size_t Sys_GetModuleSize(void *pModule)
{
#ifdef _WIN32
	auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(pModule);
	auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t *>(pModule) + dos_header->e_lfanew);
	return nt_headers->OptionalHeader.SizeOfImage;
#else
	Dl_info info;
	if (dladdr(moduleHandle, &info))
	{
		const char *modulePath = info.dli_fname;
		if (modulePath)
		{
			stat fileStats;
			if (stat(modulePath, &fileStats) == 0)
				return fileStats.st_size;
		}
	}
	return 0;
#endif
}

void *Sys_FindPattern(void *pModule, const char *pattern, void *start)
{
	std::vector<int> bytes;
	while (pattern && *pattern)
	{
		while (std::isspace(*pattern))
			pattern++;
		if (!*pattern)
			break;

		if (*pattern == '?')
		{
			pattern++;
			bytes.push_back(-1);
			if (*pattern == '?')
				pattern++;

			if (!*pattern)
				break;
			continue;
		}

		const char *current = pattern;
		bytes.push_back(std::strtoul(pattern, const_cast<char **>(&pattern), 16));
		if (current == pattern)
		{
			std::println("[Memory] Invalid pattern provided. Parsing hex digit resulted in same offset at '{}'\n", pattern);
			return nullptr;
		}
	}

	size_t size = Sys_GetModuleSize(pModule);
	if (size <= 0)
	{
		std::println("[Memory] Module {} returned 0 size\n", pModule);
		return nullptr;
	}

	uint8_t *base = reinterpret_cast<uint8_t *>(start == nullptr ? pModule : start);
	for (size_t i = 0; i <= size - bytes.size(); i++)
	{
		bool found = true;
		for (size_t j = 0; j < bytes.size(); j++)
		{
			if (bytes[j] != -1 && base[i + j] != bytes[j])
			{
				found = false;
				break;
			}
		}
		if (found)
			return base + i;
	}
	return nullptr;
}