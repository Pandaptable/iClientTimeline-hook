#include "memory.hpp"
#include "pipe_server.hpp"
#include <Windows.h>
#include <cstdint>
#include <safetyhook.hpp>
#include <thread>
#include <string>

#ifndef NDEBUG
#define LOG(...) std::println(__VA_ARGS__)
#else
#define LOG(...) ((void)0)
#endif

safetyhook::InlineHook g_ITimeline_Deserialization = {};
std::thread g_Thread = {};
bool g_quit = false;
PipeServer *g_Pipe = nullptr;

using ETimelineGameMode = int32_t;
using ETimelineEventClipPriority = int32_t;
using TimelineEventHandle_t = uint64_t;
using SteamAPICall_t = uint64_t;

class IClientTimeline
{
private:
	static inline void *sm_ptr = nullptr;
	static inline safetyhook::VmtHook sm_vmt = {};
	static inline safetyhook::VmHook sm_vm_SetTimelineTooltip = {};
	static inline safetyhook::VmHook sm_vm_ClearTimelineTooltip = {};
	static inline safetyhook::VmHook sm_vm_SetTimelineGameMode = {};
	static inline safetyhook::VmHook sm_vm_AddInstantaneousTimelineEvent = {};
	static inline safetyhook::VmHook sm_vm_AddRangeTimelineEvent = {};
	static inline safetyhook::VmHook sm_vm_StartRangeTimelineEvent = {};
	static inline safetyhook::VmHook sm_vm_UpdateRangeTimelineEvent = {};
	static inline safetyhook::VmHook sm_vm_EndRangeTimelineEvent = {};
	static inline safetyhook::VmHook sm_vm_RemoveTimelineEvent = {};
	static inline safetyhook::VmHook sm_vm_DoesEventRecordingExist = {};
	static inline safetyhook::VmHook sm_vm_StartGamePhase = {};
	static inline safetyhook::VmHook sm_vm_EndGamePhase = {};
	static inline safetyhook::VmHook sm_vm_SetGamePhaseID = {};
	static inline safetyhook::VmHook sm_vm_DoesGamePhaseRecordingExist = {};
	static inline safetyhook::VmHook sm_vm_AddGamePhaseTag = {};
	static inline safetyhook::VmHook sm_vm_SetGamePhaseAttribute = {};
	static inline safetyhook::VmHook sm_vm_OpenOverlayToGamePhase = {};
	static inline safetyhook::VmHook sm_vm_OpenOverlayToTimelineEvent = {};

public:
	static inline void SetPointer(void *ptr)
	{
		if (sm_ptr == ptr)
			return;
		LOG("New IClientTimeline pointer found: {}\n", ptr);

		sm_vm_OpenOverlayToTimelineEvent = {};
		sm_vm_OpenOverlayToGamePhase = {};
		sm_vm_SetGamePhaseAttribute = {};
		sm_vm_AddGamePhaseTag = {};
		sm_vm_DoesGamePhaseRecordingExist = {};
		sm_vm_SetGamePhaseID = {};
		sm_vm_EndGamePhase = {};
		sm_vm_StartGamePhase = {};
		sm_vm_DoesEventRecordingExist = {};
		sm_vm_RemoveTimelineEvent = {};
		sm_vm_EndRangeTimelineEvent = {};
		sm_vm_UpdateRangeTimelineEvent = {};
		sm_vm_StartRangeTimelineEvent = {};
		sm_vm_AddRangeTimelineEvent = {};
		sm_vm_AddInstantaneousTimelineEvent = {};
		sm_vm_SetTimelineGameMode = {};
		sm_vm_ClearTimelineTooltip = {};
		sm_vm_SetTimelineTooltip = {};
		sm_vmt = {};

		sm_ptr = ptr;
		if (!sm_ptr)
			return;

		sm_vmt = safetyhook::create_vmt(sm_ptr);
		sm_vm_SetTimelineTooltip = safetyhook::create_vm(sm_vmt, 0, &IClientTimeline::SetTimelineTooltip);
		sm_vm_ClearTimelineTooltip = safetyhook::create_vm(sm_vmt, 1, &IClientTimeline::ClearTimelineTooltip);
		sm_vm_SetTimelineGameMode = safetyhook::create_vm(sm_vmt, 2, &IClientTimeline::SetTimelineGameMode);
		sm_vm_AddInstantaneousTimelineEvent = safetyhook::create_vm(sm_vmt, 3, &IClientTimeline::AddInstantaneousTimelineEvent);
		sm_vm_AddRangeTimelineEvent = safetyhook::create_vm(sm_vmt, 4, &IClientTimeline::AddRangeTimelineEvent);
		sm_vm_StartRangeTimelineEvent = safetyhook::create_vm(sm_vmt, 5, &IClientTimeline::StartRangeTimelineEvent);
		sm_vm_UpdateRangeTimelineEvent = safetyhook::create_vm(sm_vmt, 6, &IClientTimeline::UpdateRangeTimelineEvent);
		sm_vm_EndRangeTimelineEvent = safetyhook::create_vm(sm_vmt, 7, &IClientTimeline::EndRangeTimelineEvent);
		sm_vm_RemoveTimelineEvent = safetyhook::create_vm(sm_vmt, 8, &IClientTimeline::RemoveTimelineEvent);
		sm_vm_DoesEventRecordingExist = safetyhook::create_vm(sm_vmt, 9, &IClientTimeline::DoesEventRecordingExist);
		sm_vm_StartGamePhase = safetyhook::create_vm(sm_vmt, 10, &IClientTimeline::StartGamePhase);
		sm_vm_EndGamePhase = safetyhook::create_vm(sm_vmt, 11, &IClientTimeline::EndGamePhase);
		sm_vm_SetGamePhaseID = safetyhook::create_vm(sm_vmt, 12, &IClientTimeline::SetGamePhaseID);
		sm_vm_DoesGamePhaseRecordingExist = safetyhook::create_vm(sm_vmt, 13, &IClientTimeline::DoesGamePhaseRecordingExist);
		sm_vm_AddGamePhaseTag = safetyhook::create_vm(sm_vmt, 14, &IClientTimeline::AddGamePhaseTag);
		sm_vm_SetGamePhaseAttribute = safetyhook::create_vm(sm_vmt, 15, &IClientTimeline::SetGamePhaseAttribute);
		sm_vm_OpenOverlayToGamePhase = safetyhook::create_vm(sm_vmt, 16, &IClientTimeline::OpenOverlayToGamePhase);
		sm_vm_OpenOverlayToTimelineEvent = safetyhook::create_vm(sm_vmt, 17, &IClientTimeline::OpenOverlayToTimelineEvent);
	}

public:
	void SetTimelineTooltip(const char *pchDescription, float flTimeDelta)
	{
		LOG("IClientTimeline::SetTimelineTooltip(\"{}\", {})\n", pchDescription ? pchDescription : "<nullptr>", flTimeDelta);
		sm_vm_SetTimelineTooltip.thiscall<void>(this, pchDescription, flTimeDelta);
		if (g_Pipe)
			g_Pipe->Write("SetTimelineTooltip", {{"description", pchDescription ? pchDescription : ""}, {"timeDelta", flTimeDelta}});
	}

	void ClearTimelineTooltip(float flTimeDelta)
	{
		LOG("IClientTimeline::ClearTimelineTooltip({})\n", flTimeDelta);
		sm_vm_ClearTimelineTooltip.thiscall<void>(this, flTimeDelta);
		if (g_Pipe)
			g_Pipe->Write("ClearTimelineTooltip", {{"timeDelta", flTimeDelta}});
	}

	void SetTimelineGameMode(ETimelineGameMode eMode)
	{
		LOG("IClientTimeline::SetTimelineGameMode({})\n", static_cast<int>(eMode));
		sm_vm_SetTimelineGameMode.thiscall<void>(this, eMode);
		if (g_Pipe)
			g_Pipe->Write("SetTimelineGameMode", {{"mode", static_cast<int>(eMode)}});
	}

	TimelineEventHandle_t AddInstantaneousTimelineEvent(const char *pchTitle, const char *pchDescription, const char *pchIcon, uint32_t unIconPriority, float flStartOffsetSeconds = 0.f, ETimelineEventClipPriority ePossibleClip = 1)
	{
		LOG("IClientTimeline::AddInstantaneousTimelineEvent(\"{}\", \"{}\", \"{}\", {}, {}, {})\n", pchTitle ? pchTitle : "<nullptr>", pchDescription ? pchDescription : "<nullptr>", pchIcon ? pchIcon : "<nullptr>", unIconPriority, flStartOffsetSeconds, static_cast<int>(ePossibleClip));
		auto handle = sm_vm_AddInstantaneousTimelineEvent.thiscall<TimelineEventHandle_t>(this, pchTitle, pchDescription, pchIcon, unIconPriority, flStartOffsetSeconds, ePossibleClip);
		if (g_Pipe)
			g_Pipe->Write("AddInstantaneousTimelineEvent", {{"title", pchTitle ? pchTitle : ""}, {"description", pchDescription ? pchDescription : ""}, {"icon", pchIcon ? pchIcon : ""}, {"priority", unIconPriority}, {"startOffset", flStartOffsetSeconds}, {"clipPriority", static_cast<int>(ePossibleClip)}, {"handle", handle}});
		return handle;
	}

	TimelineEventHandle_t AddRangeTimelineEvent(const char *pchTitle, const char *pchDescription, const char *pchIcon, uint32_t unIconPriority, float flStartOffsetSeconds, float flDuration, ETimelineEventClipPriority ePossibleClip)
	{
		LOG("IClientTimeline::AddRangeTimelineEvent(\"{}\", \"{}\", \"{}\", {}, {}, {}, {})\n", pchTitle ? pchTitle : "<nullptr>", pchDescription ? pchDescription : "<nullptr>", pchIcon ? pchIcon : "<nullptr>", unIconPriority, flStartOffsetSeconds, flDuration, static_cast<int>(ePossibleClip));
		auto handle = sm_vm_AddRangeTimelineEvent.thiscall<TimelineEventHandle_t>(this, pchTitle, pchDescription, pchIcon, unIconPriority, flStartOffsetSeconds, flDuration, ePossibleClip);
		if (g_Pipe)
			g_Pipe->Write("AddRangeTimelineEvent", {{"title", pchTitle ? pchTitle : ""}, {"description", pchDescription ? pchDescription : ""}, {"icon", pchIcon ? pchIcon : ""}, {"priority", unIconPriority}, {"startOffset", flStartOffsetSeconds}, {"duration", flDuration}, {"clipPriority", static_cast<int>(ePossibleClip)}, {"handle", handle}});
		return handle;
	}

	TimelineEventHandle_t StartRangeTimelineEvent(const char *pchTitle, const char *pchDescription, const char *pchIcon, uint32_t unPriority, float flStartOffsetSeconds, ETimelineEventClipPriority ePossibleClip)
	{
		LOG("IClientTimeline::StartRangeTimelineEvent(\"{}\", \"{}\", \"{}\", {}, {}, {})\n", pchTitle ? pchTitle : "<nullptr>", pchDescription ? pchDescription : "<nullptr>", pchIcon ? pchIcon : "<nullptr>", unPriority, flStartOffsetSeconds, static_cast<int>(ePossibleClip));
		auto handle = sm_vm_StartRangeTimelineEvent.thiscall<TimelineEventHandle_t>(this, pchTitle, pchDescription, pchIcon, unPriority, flStartOffsetSeconds, ePossibleClip);
		if (g_Pipe)
			g_Pipe->Write("StartRangeTimelineEvent", {{"title", pchTitle ? pchTitle : ""}, {"description", pchDescription ? pchDescription : ""}, {"icon", pchIcon ? pchIcon : ""}, {"priority", unPriority}, {"startOffset", flStartOffsetSeconds}, {"clipPriority", static_cast<int>(ePossibleClip)}, {"handle", handle}});
		return handle;
	}

	void UpdateRangeTimelineEvent(TimelineEventHandle_t ulEvent, const char *pchTitle, const char *pchDescription, const char *pchIcon, uint32_t unPriority, ETimelineEventClipPriority ePossibleClip)
	{
		LOG("IClientTimeline::UpdateRangeTimelineEvent({}, \"{}\", \"{}\", \"{}\", {}, {})\n", ulEvent, pchTitle ? pchTitle : "<nullptr>", pchDescription ? pchDescription : "<nullptr>", pchIcon ? pchIcon : "<nullptr>", unPriority, static_cast<int>(ePossibleClip));
		sm_vm_UpdateRangeTimelineEvent.thiscall<void>(this, ulEvent, pchTitle, pchDescription, pchIcon, unPriority, ePossibleClip);
		if (g_Pipe)
			g_Pipe->Write("UpdateRangeTimelineEvent", {{"handle", ulEvent}, {"title", pchTitle ? pchTitle : ""}, {"description", pchDescription ? pchDescription : ""}, {"icon", pchIcon ? pchIcon : ""}, {"priority", unPriority}, {"clipPriority", static_cast<int>(ePossibleClip)}});
	}

	void EndRangeTimelineEvent(TimelineEventHandle_t ulEvent, float flEndOffsetSeconds)
	{
		LOG("IClientTimeline::EndRangeTimelineEvent({}, {})\n", ulEvent, flEndOffsetSeconds);
		sm_vm_EndRangeTimelineEvent.thiscall<void>(this, ulEvent, flEndOffsetSeconds);
		if (g_Pipe)
			g_Pipe->Write("EndRangeTimelineEvent", {{"handle", ulEvent}, {"endOffset", flEndOffsetSeconds}});
	}

	void RemoveTimelineEvent(TimelineEventHandle_t ulEvent)
	{
		LOG("IClientTimeline::RemoveTimelineEvent({})\n", ulEvent);
		sm_vm_RemoveTimelineEvent.thiscall<void>(this, ulEvent);
		if (g_Pipe)
			g_Pipe->Write("RemoveTimelineEvent", {{"handle", ulEvent}});
	}

	SteamAPICall_t DoesEventRecordingExist(TimelineEventHandle_t ulEvent)
	{
		LOG("IClientTimeline::DoesEventRecordingExist({})\n", ulEvent);
		auto result = sm_vm_DoesEventRecordingExist.thiscall<SteamAPICall_t>(this, ulEvent);
		if (g_Pipe)
			g_Pipe->Write("DoesEventRecordingExist", {{"handle", ulEvent}});
		return result;
	}

	void StartGamePhase()
	{
		LOG("IClientTimeline::StartGamePhase()\n");
		sm_vm_StartGamePhase.thiscall<void>(this);
		if (g_Pipe)
			g_Pipe->Write("StartGamePhase");
	}

	void EndGamePhase()
	{
		LOG("IClientTimeline::EndGamePhase()\n");
		sm_vm_EndGamePhase.thiscall<void>(this);
		if (g_Pipe)
			g_Pipe->Write("EndGamePhase");
	}

	void SetGamePhaseID(const char *pchPhaseID)
	{
		LOG("IClientTimeline::SetGamePhaseID(\"{}\")\n", pchPhaseID ? pchPhaseID : "<nullptr>");
		sm_vm_SetGamePhaseID.thiscall<void>(this, pchPhaseID);
		if (g_Pipe)
			g_Pipe->Write("SetGamePhaseID", {{"phaseID", pchPhaseID ? pchPhaseID : ""}});
	}

	SteamAPICall_t DoesGamePhaseRecordingExist(const char *pchPhaseID)
	{
		LOG("IClientTimeline::DoesGamePhaseRecordingExist(\"{}\")\n", pchPhaseID ? pchPhaseID : "<nullptr>");
		auto result = sm_vm_DoesGamePhaseRecordingExist.thiscall<SteamAPICall_t>(this, pchPhaseID);
		if (g_Pipe)
			g_Pipe->Write("DoesGamePhaseRecordingExist", {{"phaseID", pchPhaseID ? pchPhaseID : ""}});
		return result;
	}

	void AddGamePhaseTag(const char *pchTagName, const char *pchTagIcon, const char *pchTagGroup, uint32_t unPriority)
	{
		LOG("IClientTimeline::AddGamePhaseTag(\"{}\", \"{}\", \"{}\", {})\n", pchTagName ? pchTagName : "<nullptr>", pchTagIcon ? pchTagIcon : "<nullptr>", pchTagGroup ? pchTagGroup : "<nullptr>", unPriority);
		sm_vm_AddGamePhaseTag.thiscall<void>(this, pchTagName, pchTagIcon, pchTagGroup, unPriority);
		if (g_Pipe)
			g_Pipe->Write("AddGamePhaseTag", {{"tagName", pchTagName ? pchTagName : ""}, {"tagIcon", pchTagIcon ? pchTagIcon : ""}, {"tagGroup", pchTagGroup ? pchTagGroup : ""}, {"priority", unPriority}});
	}

	void SetGamePhaseAttribute(const char *pchAttributeGroup, const char *pchAttributeValue, uint32_t unPriority)
	{
		LOG("IClientTimeline::SetGamePhaseAttribute(\"{}\", \"{}\", {})\n", pchAttributeGroup ? pchAttributeGroup : "<nullptr>", pchAttributeValue ? pchAttributeValue : "<nullptr>", unPriority);
		sm_vm_SetGamePhaseAttribute.thiscall<void>(this, pchAttributeGroup, pchAttributeValue, unPriority);
		if (g_Pipe)
			g_Pipe->Write("SetGamePhaseAttribute", {{"attributeGroup", pchAttributeGroup ? pchAttributeGroup : ""}, {"attributeValue", pchAttributeValue ? pchAttributeValue : ""}, {"priority", unPriority}});
	}

	void OpenOverlayToGamePhase(const char *pchPhaseID)
	{
		LOG("IClientTimeline::OpenOverlayToGamePhase(\"{}\")\n", pchPhaseID ? pchPhaseID : "<nullptr>");
		sm_vm_OpenOverlayToGamePhase.thiscall<void>(this, pchPhaseID);
		if (g_Pipe)
			g_Pipe->Write("OpenOverlayToGamePhase", {{"phaseID", pchPhaseID ? pchPhaseID : ""}});
	}

	void OpenOverlayToTimelineEvent(const TimelineEventHandle_t ulEvent)
	{
		LOG("IClientTimeline::OpenOverlayToTimelineEvent({})\n", ulEvent);
		sm_vm_OpenOverlayToTimelineEvent.thiscall<void>(this, ulEvent);
		if (g_Pipe)
			g_Pipe->Write("OpenOverlayToTimelineEvent", {{"handle", ulEvent}});
	}
};

static void *__fastcall ITimeline_Deserialization(void *ptr, void *unk1, void *unk2, void *unk3)
{
	IClientTimeline::SetPointer(ptr);
	return g_ITimeline_Deserialization.fastcall<void *>(ptr, unk1, unk2, unk3);
}

void MainThread(HMODULE hModule, FILE *fDummy)
{
	HMODULE hSteamClient = nullptr;
	while (!g_quit && !(hSteamClient = GetModuleHandle("steamclient64.dll")))
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

	if (hSteamClient)
	{
		// horrifying pattern someone please PR a better one lmfao
		void *ptr = Sys_FindPattern(reinterpret_cast<void *>(hSteamClient), "40 55 53 56 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B FA 49 8B F0 4C 8B F1 48 8D 55 ? 48 8B CF 41 B8 ? ? ? ? 49 8B D9 E8 ? ? ? ? 8B 45 ? 3D ? ? ? ? 0F 87 ? ? ? ? 0F 84 ? ? ? ? 3D ? ? ? ? 0F 87 ? ? ? ? 0F 84 ? ? ? ? 3D ? ? ? ? 0F 87");
		if (ptr)
		{
			g_ITimeline_Deserialization = safetyhook::create_inline(ptr, &ITimeline_Deserialization);
		}
		else
		{
			LOG("Failed to find IClientTimeline signature\n");
		}
	}

	while (!g_quit && !(GetAsyncKeyState(VK_END) & 0x8000))
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	LOG("Quit\n");

	IClientTimeline::SetPointer(nullptr);
	g_ITimeline_Deserialization = {};

	if (g_Pipe)
	{
		g_Pipe->Stop();
		delete g_Pipe;
		g_Pipe = nullptr;
	}

#ifndef NDEBUG
	if (fDummy)
		fclose(fDummy);
	FreeConsole();
#else
	(void)fDummy;
#endif

	FreeLibraryAndExitThread(hModule, 0);
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);

		char exePath[MAX_PATH];
		if (GetModuleFileNameA(NULL, exePath, MAX_PATH))
		{
			const char *exeName = strrchr(exePath, '\\');
			exeName = exeName ? exeName + 1 : exePath;

			if (_stricmp(exeName, "steam.exe") != 0)
			{
				return TRUE;
			}
		}

		std::string dllList = HOOK_DLLS;
		size_t start = 0;
		size_t end = dllList.find('|');
		while (end != std::string::npos)
		{
			if (end != start)
				LoadLibraryA(dllList.substr(start, end - start).c_str());
			start = end + 1;
			end = dllList.find('|', start);
		}
		if (start < dllList.length())
			LoadLibraryA(dllList.substr(start).c_str());

		FILE *fDummy = nullptr;
#ifndef NDEBUG
		AllocConsole();
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
#endif

		g_Pipe = new PipeServer();
		g_Pipe->Start();

		g_Thread = std::thread(MainThread, hModule, fDummy);
	}
	else if (fdwReason == DLL_PROCESS_DETACH && lpvReserved == nullptr)
	{
		g_quit = true;
		if (g_Thread.joinable())
			g_Thread.join();
	}
	return TRUE;
}
