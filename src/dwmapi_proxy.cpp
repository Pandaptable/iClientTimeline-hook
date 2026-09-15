// https://github.com/hazre/OpenSteamTool/commit/cb52df0cc9830819aba323ddd9de1bc3f4ad85ad#diff-b4e7f6cc509ad838b2196a6ae058ee5247ceb7b71fb6bd0c0e447b7b0bbbdec2

#include <Windows.h>
#include <cstdint>
#include <string>

struct DwmProxyTarget
{
	const char *name;
	int ordinal;
	void **slot;
};

#include "dwmapi_targets.inc"

extern "C" __int64 DwmProxy_Unresolved()
{
	return 0;
}

static HMODULE g_hRealDwmapi = nullptr;

void DwmProxy_Init()
{
	if (g_hRealDwmapi)
		return;

	char sysDir[MAX_PATH];
	if (!GetSystemDirectoryA(sysDir, MAX_PATH))
		return;
	std::string realPath = std::string(sysDir) + "\\dwmapi.dll";

	g_hRealDwmapi = LoadLibraryExA(realPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

	for (const auto &t : kDwmProxyTargets)
	{
		void *target = nullptr;
		if (g_hRealDwmapi)
		{
			target = reinterpret_cast<void *>(
				GetProcAddress(g_hRealDwmapi, t.name ? t.name : MAKEINTRESOURCEA(t.ordinal)));
		}
		*t.slot = target ? target : reinterpret_cast<void *>(&DwmProxy_Unresolved);
	}
}
