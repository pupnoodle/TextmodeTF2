#include "Core.h"

#include "../SDK/SDK.h"
#include "../BytePatches/BytePatches.h"
#include "../Hooks/VPhysics.h"
#include <filesystem>

#define LOAD_WAIT 0 - m_bTimeout
#define LOAD_FAIL -1
#define CHECK(x, sFailMessage) if (x == LOAD_FAIL) {m_bUnload = true; SDK::Output("TextmodeTF2", sFailMessage); return;}

namespace
{
	constexpr DWORD kSteamModulePollIntervalMs = 100;
	constexpr DWORD kSteamModuleLogIntervalMs = 5'000;

	bool IsSteamModuleReady()
	{
		const bool hasSteamApi = GetModuleHandleA("steam_api64.dll");
		const bool hasSteamClient = GetModuleHandleA("steamclient64.dll");
		return hasSteamApi && hasSteamClient;
	}

	void WaitForSteamModules()
	{
		bool loggedWait = false;
		DWORD lastLogTick = GetTickCount();

		while (!IsSteamModuleReady())
		{
			if (!loggedWait)
			{
				SDK::Output("Core", "Waiting for Steam modules before continuing TextmodeTF2 initialization...");
				loggedWait = true;
				lastLogTick = GetTickCount();
			}
			else
			{
				const DWORD now = GetTickCount();
				if (now - lastLogTick >= kSteamModuleLogIntervalMs)
				{
					SDK::Output("Core", "Still waiting for steam_api/steamclient modules...");
					lastLogTick = now;
				}
			}

			Sleep(kSteamModulePollIntervalMs);
		}

		if (loggedWait)
			SDK::Output("Core", "Steam modules detected. Resuming TextmodeTF2 initialization.");
	}
}

void CCore::AppendFailText(const char* sMessage, bool bCritical)
{
	if (!m_bTimeout && !bCritical)
		return;

	ssFailStream << std::format("{}\n", sMessage);
	SDK::Output(sMessage);
}

void CCore::AppendSuccessText(const char* sFunction, const char* sMessage)
{
	SDK::Output(sFunction, sMessage);
}

int CCore::LoadFilesystem()
{
	G::IFileSystemAddr = reinterpret_cast<uintptr_t>(U::Memory.FindInterface("filesystem_stdio.dll", "VFileSystem022"));
	if (!G::IFileSystemAddr)
		return LOAD_WAIT;

	G::IBaseFileSystemAddr = G::IFileSystemAddr + 0x8;

	static std::vector<const char*> vFilesystemHooks
	{
		"IFileSystem_FindFirst", "IFileSystem_FindNext",
		"IFileSystem_AsyncReadMultiple", "IFileSystem_OpenEx",
		"IFileSystem_ReadFileEx", "IFileSystem_AddFilesToFileCache",
		"IBaseFileSystem_Open", "IBaseFileSystem_Precache",
		"IBaseFileSystem_ReadFile", "IBaseFileSystem_FileExists"
	};

	for (auto cHook : vFilesystemHooks)
		if (!U::Hooks.Initialize(cHook))
			return LOAD_FAIL;

	return m_bFilesystemLoaded = true;
}

int CCore::LoadEngine()
{
	static bool bTextmodeInit{ false };
	if(!G::g_bTextModeAddr)
		G::g_bTextModeAddr = U::Memory.FindSignature("engine.dll", "88 15 ? ? ? ? 48 8B 4E");
	if (!bTextmodeInit && G::g_bTextModeAddr)
		*reinterpret_cast<bool*>(U::Memory.RelToAbs(G::g_bTextModeAddr, 0x2)) = bTextmodeInit = true;

	static bool bStartupGraphicHookInit{ false };
	if (!G::CVideoModeCommon_SetupStartupGraphicAddr)
		G::CVideoModeCommon_SetupStartupGraphicAddr = U::Memory.FindSignature("engine.dll", "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B F9");
	if (!bStartupGraphicHookInit && G::CVideoModeCommon_SetupStartupGraphicAddr)
	{
		if (!U::Hooks.Initialize("CVideoModeCommon_SetupStartupGraphic"))
			return LOAD_FAIL;
		bStartupGraphicHookInit = true;
	}

	static bool bInsecureBypassInit{ false };
	if (!bInsecureBypassInit)
	{
		if (!G::g_bAllowSecureServersAddr || !G::Host_IsSecureServerAllowedAddr)
		{
			G::g_bAllowSecureServersAddr = U::Memory.FindSignature("engine.dll", "40 88 35 ? ? ? ? 40 84 FF");
			G::Host_IsSecureServerAllowedAddr = U::Memory.FindSignature("engine.dll", "48 83 EC ? FF 15 ? ? ? ? 48 8D 15 ? ? ? ? 48 8B C8 4C 8B 00 41 FF 50 ? 85 C0 75");
		}
		if (G::g_bAllowSecureServersAddr && G::Host_IsSecureServerAllowedAddr)
		{
			if (!U::Hooks.Initialize("Host_IsSecureServerAllowed"))
				return LOAD_FAIL;
			bInsecureBypassInit = true;
		}
	}

	static bool bCon_DebugLogInit{ false };
	if (!bCon_DebugLogInit)
	{
		if (!G::Con_DebugLogAddr)
			G::Con_DebugLogAddr = U::Memory.FindSignature("engine.dll", "48 89 4C 24 ? 48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? 57");
		if (G::Con_DebugLogAddr)
		{
			if (!U::Hooks.Initialize("Con_DebugLog"))
				return LOAD_FAIL;
			bCon_DebugLogInit = true;
		}
	}

	static bool bAudioHooksInit{ false };
	if (!bAudioHooksInit)
	{
		if (U::Memory.FindInterface("engine.dll", "VEngineClient013"))
		{
			if (!U::Hooks.Initialize("CEngineClient_Sound_ExtraUpdate"))
				return LOAD_FAIL;
			bAudioHooksInit = true;
		}
	}

	static bool bRenderHooksInit{ false };
	if (!bRenderHooksInit)
	{
		if (!G::CStaticPropMgr_DrawStaticPropsAddr)
			G::CStaticPropMgr_DrawStaticPropsAddr = U::Memory.FindSignature("engine.dll", "4C 8B DC 49 89 5B 08 49 89 6B 10 49 89 73 18 57 41 54 41 55 41 56 41 57 48 83 EC 70 4C 8B 3D ? ? ? ? 33 FF");
		if (!G::CStaticPropMgr_UnserializeStaticPropsAddr)
			G::CStaticPropMgr_UnserializeStaticPropsAddr = U::Memory.FindSignature("engine.dll", "40 57 48 81 EC E0 00 00 00 80 B9 98 00 00 00 00");
		if (!G::S_PrecacheSoundAddr)
			G::S_PrecacheSoundAddr = U::Memory.FindSignature("engine.dll", "4C 8B DC 49 89 5B ? 49 89 73 ? 57 48 83 EC ? 48 8B 05 ? ? ? ? 33 F6");
		if (!G::CModelLoader_GetModelForNameAddr)
			G::CModelLoader_GetModelForNameAddr = U::Memory.FindSignature("engine.dll", "44 89 44 24 18 53 48 83 EC 20 48 8B D9 E8 ?? ?? ?? ?? 4C 8D 44 24 40");
		if (!G::CModelRender_DrawModelExecuteAddr)
			G::CModelRender_DrawModelExecuteAddr = U::Memory.FindSignature("engine.dll", "4C 89 4C 24 20 48 89 4C 24 08 55 53 56 57 41 54 41 56 41 57 48 8D AC 24 D0 FD FF FF 48 81 EC 30 03 00 00 41 8B 70 40");
		if (!G::CDebugOverlay_AddBoxOverlayAddr)
			G::CDebugOverlay_AddBoxOverlayAddr = U::Memory.FindSignature("engine.dll", "48 89 74 24 18 4C 89 74 24 20 41 57 48 81 EC 80 00 00 00 48 8D 0D ?? ?? ?? ?? 49 8B F1 4D 8B F0 4C 8B FA E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 48 89 9C 24 90 00 00 00 4C 8D 0D ?? ?? ?? ?? 0F 29 74 24 70 4C 8D 05 ?? ?? ?? ?? C7 44 24 28 01 00 00 00 48 8D 15 ?? ?? ?? ?? 48 8D 4C 24 40 48 89 BC 24 98 00 00 00 C7 44 24 20 61 02 00 00 E8 ?? ?? ?? ?? B9 58 00 00");
		if (!G::CDebugOverlay_AddLineOverlayAddr)
			G::CDebugOverlay_AddLineOverlayAddr = U::Memory.FindSignature("engine.dll", "4C 8B DC 53 55 56 57 41 56 48 81 EC 80 00 00 00 49 8D 43 30 0F 29 74 24 70 48 89 81 10 04 00 00 48 8D 69 10");

		if (G::CStaticPropMgr_DrawStaticPropsAddr && G::CStaticPropMgr_UnserializeStaticPropsAddr && G::S_PrecacheSoundAddr && G::CModelLoader_GetModelForNameAddr && G::CModelRender_DrawModelExecuteAddr && G::CDebugOverlay_AddBoxOverlayAddr && G::CDebugOverlay_AddLineOverlayAddr)
		{
			if (!U::Hooks.Initialize("CStaticPropMgr_DrawStaticProps")) return LOAD_FAIL;
			if (!U::Hooks.Initialize("CStaticPropMgr_UnserializeStaticProps")) return LOAD_FAIL;
			if (!U::Hooks.Initialize("S_PrecacheSound")) return LOAD_FAIL;
			if (!U::Hooks.Initialize("CModelLoader_GetModelForName")) return LOAD_FAIL;
			if (!U::Hooks.Initialize("CModelRender_DrawModelExecute")) return LOAD_FAIL;
			if (!U::Hooks.Initialize("CDebugOverlay_AddBoxOverlay")) return LOAD_FAIL;
			if (!U::Hooks.Initialize("CDebugOverlay_AddLineOverlay")) return LOAD_FAIL;
			bRenderHooksInit = true;
		}

		if (!G::CEngineSoundClient_EmitSoundInternalAddr)
			G::CEngineSoundClient_EmitSoundInternalAddr = U::Memory.FindSignature("engine.dll", "48 8B C4 44 89 48 20 55 41 57 48 8D 68 C8 48 81 EC ?? ?? ?? ??");
		if (G::CEngineSoundClient_EmitSoundInternalAddr)
			U::Hooks.Initialize("CEngineSoundClient_EmitSoundInternal");

		if (!G::R_DrawDecalsAllAddr)
			G::R_DrawDecalsAllAddr = U::Memory.FindSignature("engine.dll", "48 89 4C 24 08 55 48 8D AC 24 40 F0 FF FF B8 C0 10 00 00 E8");
		if (G::R_DrawDecalsAllAddr)
			U::Hooks.Initialize("R_DrawDecalsAll");

		if (!G::CShadowMgr_RenderShadowsAddr)
			G::CShadowMgr_RenderShadowsAddr = U::Memory.FindSignature("engine.dll", "4C 8B DC 55 41 57 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 45 33 FF");
		if (G::CShadowMgr_RenderShadowsAddr)
			U::Hooks.Initialize("CShadowMgr_RenderShadows");

		// temp disabled
		// if (!G::SVC_Sounds_ProcessAddr)
		// 	G::SVC_Sounds_ProcessAddr = U::Memory.FindSignature("engine.dll", "48 8B D1 48 8B 49 20 48 8B 01 48 FF A0 78 00 00 00");
		// if (G::SVC_Sounds_ProcessAddr)
		// 	U::Hooks.Initialize("SVC_Sounds_Process");

		if (!G::SVC_BSPDecal_ProcessAddr)
			G::SVC_BSPDecal_ProcessAddr = U::Memory.FindSignature("engine.dll", "48 8B D1 48 8B 49 20 48 8B 01 48 FF A0 98 00 00 00");
		if (G::SVC_BSPDecal_ProcessAddr)
			U::Hooks.Initialize("SVC_BSPDecal_Process");

	}

	static bool bBytePatchesInit{ false };
	if (!bBytePatchesInit && U::BytePatches.Initialize("engine"))
		bBytePatchesInit = true;


	if(!bStartupGraphicHookInit || !bInsecureBypassInit || !bTextmodeInit || !bCon_DebugLogInit || !bAudioHooksInit || !bBytePatchesInit || !bRenderHooksInit)
		return LOAD_WAIT;

	return m_bEngineLoaded = true;
}

int CCore::LoadMatSys()
{
	if (!U::Interfaces.Initialize())
		return LOAD_WAIT;

	I::MaterialSystem->SetInStubMode(true);

	static std::vector<const char*> vMatSystemHooks
	{
		"IMaterialSystem_CreateRenderTargetTexture", "IMaterialSystem_CreateNamedRenderTargetTextureEx",
		"IMaterialSystem_CreateNamedRenderTargetTexture", "IMaterialSystem_CreateNamedRenderTargetTextureEx2",
		"IMaterialSystem_SwapBuffers", "IMaterialSystem_CacheUsedMaterials",
		"IMaterialSystem_FindMaterial", "IMaterialSystem_FindTexture"
	};

	for (auto cHook : vMatSystemHooks)
		if (!U::Hooks.Initialize(cHook))
			return LOAD_FAIL;

	if (I::StudioRender)
	{
		G::IStudioRender_DrawModelAddr = reinterpret_cast<uintptr_t>(U::Memory.GetVFunc(I::StudioRender, 29));
		G::IStudioRender_DrawModelStaticPropAddr = reinterpret_cast<uintptr_t>(U::Memory.GetVFunc(I::StudioRender, 30));
		G::IStudioRender_DrawStaticPropDecalsAddr = reinterpret_cast<uintptr_t>(U::Memory.GetVFunc(I::StudioRender, 31));
		G::IStudioRender_DrawStaticPropShadowsAddr = reinterpret_cast<uintptr_t>(U::Memory.GetVFunc(I::StudioRender, 32));
		G::IStudioRender_AddDecalAddr = reinterpret_cast<uintptr_t>(U::Memory.GetVFunc(I::StudioRender, 36));
		G::IStudioRender_AddShadowAddr = reinterpret_cast<uintptr_t>(U::Memory.GetVFunc(I::StudioRender, 39));
		G::IStudioRender_DrawModelArrayAddr = reinterpret_cast<uintptr_t>(U::Memory.GetVFunc(I::StudioRender, 46));

		U::Hooks.Initialize("IStudioRender_DrawModel");
		U::Hooks.Initialize("IStudioRender_DrawModelStaticProp");
		U::Hooks.Initialize("IStudioRender_DrawStaticPropDecals");
		U::Hooks.Initialize("IStudioRender_DrawStaticPropShadows");
		U::Hooks.Initialize("IStudioRender_AddDecal");
		U::Hooks.Initialize("IStudioRender_AddShadow");
		U::Hooks.Initialize("IStudioRender_DrawModelArray");
	}

	if (!U::BytePatches.Initialize("materialsystem"))
		return LOAD_FAIL;

	return m_bMatSysLoaded = true;
}

int CCore::LoadClient()
{
	static bool bClientCrashHooksInit{ false };
	if (!bClientCrashHooksInit)
	{
		if (!G::Client_SafeHandleLookupAddr)
		{
			if (const auto hClient = GetModuleHandleA("client.dll"))
				G::Client_SafeHandleLookupAddr = reinterpret_cast<uintptr_t>(hClient) + 0x57C2C0;
		}

		if (!G::Client_SafeMatrixTransformAddr)
			G::Client_SafeMatrixTransformAddr = U::Memory.FindSignature("client.dll", "48 83 EC ? 0F 10 19 0F 10 61 ? 0F 10 6A ? 0F 29 74 24 ? 0F 28 D3 0F 10 72 ?");

		if (G::Client_SafeHandleLookupAddr && G::Client_SafeMatrixTransformAddr)
		{
			if (!U::Hooks.Initialize("Client_SafeHandleLookup"))
				return LOAD_FAIL;
			if (!U::Hooks.Initialize("Client_SafeMatrixTransform"))
				return LOAD_FAIL;

			bClientCrashHooksInit = true;
		}
	}

	if (!U::BytePatches.Initialize("client"))
		return LOAD_WAIT;

	static bool bClientAudioHooksInit{ false };
	if (!bClientAudioHooksInit)
	{
		static std::vector<const char*> vClientAudioHooks
		{
			"C_SoundscapeSystem_Init",
			"C_SoundscapeSystem_Update",
			"C_SoundscapeSystem_UpdateAudioParams",
			"CMumbleSystem_LevelInitPostEntity",
			"CMumbleSystem_PostRender"
		};

		for (auto cHook : vClientAudioHooks)
			if (!U::Hooks.Initialize(cHook))
				return LOAD_FAIL;

		bClientAudioHooksInit = true;
	}

	// IPanel::PaintTraverse
	if (!G::IPanel_PaintTraverseAddr)
	{
		if (auto pPanel = U::Memory.FindInterface("vgui2.dll", "VGUI_Panel009"))
			G::IPanel_PaintTraverseAddr = reinterpret_cast<uintptr_t>(U::Memory.GetVFunc(pPanel, 41));
	}
	if (G::IPanel_PaintTraverseAddr)
		U::Hooks.Initialize("IPanel_PaintTraverse");

	return m_bClientLoaded = bClientCrashHooksInit;
}

int CCore::LoadGameUI()
{
	if (!GetModuleHandleA("GameUI.dll"))
		return LOAD_WAIT;

	if (!U::BytePatches.Initialize("gameui"))
		return LOAD_FAIL;

	return m_bGameUILoaded = true;
}

int CCore::LoadParticles()
{
	return m_bParticlesLoaded = true;
}

int CCore::LoadMDLCache()
{
	G::IMDLCache = reinterpret_cast<IMDLCache*>(U::Memory.FindInterface("datacache.dll", "MDLCache004"));
	if (!G::IMDLCache)
		return LOAD_WAIT;

	if (!U::Hooks.Initialize("IMDLCache_ProcessDataIntoCache"))
		return LOAD_FAIL;

	if (!U::Hooks.Initialize("IMDLCache_GetHardwareData"))
		return LOAD_FAIL;

	if (!U::Hooks.Initialize("IMDLCache_GetVertexData"))
		return LOAD_FAIL;

	if (!U::Hooks.Initialize("IMDLCache_TouchAllData"))
		return LOAD_FAIL;

	if (!U::BytePatches.Initialize("datacache"))
		return LOAD_FAIL;

	return m_bMDLCacheLoaded = true;
}

int CCore::LoadVideoServices()
{
	I::VideoServices = reinterpret_cast<IVideoServices*>(U::Memory.FindInterface("video_services.dll", "IVideoServices002"));
	if (!I::VideoServices)
		return LOAD_WAIT;

	if (!U::Hooks.Initialize("IVideoServices_CreateVideoMaterial"))
		return LOAD_FAIL;

	if (!U::Hooks.Initialize("IVideoServices_PlayVideoFileFullScreen"))
		return LOAD_FAIL;

	return m_bVideoServicesLoaded = true;
}

int CCore::LoadVPhysics()
{
	G::Physics = reinterpret_cast<IPhysics*>(U::Memory.FindInterface("vphysics.dll", "VPhysics031"));
	if (!G::Physics)
		return LOAD_WAIT;

	Hooks::VPhysics::Initialize();

	return m_bVPhysicsLoaded = true;
}

int CCore::LoadReplay()
{
	if (!GetModuleHandleA("replay.dll"))
		return 1;

	if (!U::BytePatches.Initialize("replay"))
		return LOAD_FAIL;

	return m_bReplayLoaded = true;
}

void CCore::Load()
{
	WaitForSteamModules();

	G::CurrentPath = std::filesystem::current_path().string() + "\\TextmodeTF2";
	SDK::Output("Core", "Initializing logging...");
	char* cBotID = nullptr;
	if (_dupenv_s(&cBotID, nullptr, "BOTID") == 0 && cBotID)
	{
		char* cAppdataPath = nullptr;
		if (_dupenv_s(&cAppdataPath, nullptr, "LOCALAPPDATA") == 0 && cAppdataPath)
		{
			G::AppdataPath = std::format("{}\\{}_{}\\", cAppdataPath, "Amalgam\\Textmode", cBotID);
			free(cAppdataPath);
		}
		free(cBotID);
	}
	if (G::AppdataPath.size())
	{
		try
		{
			if (!std::filesystem::exists(G::AppdataPath))
				std::filesystem::create_directories(G::AppdataPath);

			std::ofstream resetFile(G::AppdataPath + TEXTMODE_LOG_FILE);
			resetFile.close();
			resetFile.open(G::AppdataPath + CONSOLE_LOG_FILE);
			resetFile.close();
		}
		catch (...) {}
	}

	const auto hasRequiredModules = []()
	{
		return GetModuleHandleA("filesystem_stdio.dll") &&
			GetModuleHandleA("engine.dll") &&
			GetModuleHandleA("materialsystem.dll") &&
			GetModuleHandleA("client.dll") &&
			GetModuleHandleA("GameUI.dll") &&
			GetModuleHandleA("datacache.dll") &&
			GetModuleHandleA("video_services.dll") &&
			GetModuleHandleA("vphysics.dll");
	};

	const auto isInitializationComplete = [&]()
	{
		const bool bReplayReady = !GetModuleHandleA("replay.dll") || m_bReplayLoaded;
		return m_bFilesystemLoaded &&
			m_bEngineLoaded &&
			m_bMatSysLoaded &&
			m_bClientLoaded &&
			m_bGameUILoaded &&
			m_bParticlesLoaded &&
			m_bMDLCacheLoaded &&
			m_bVideoServicesLoaded &&
			m_bVPhysicsLoaded &&
			bReplayReady;
	};

	do
	{
		// if all required modules are loaded and we still fail stop trying to load
		m_bTimeout = hasRequiredModules();

		int iFilesystem = m_bFilesystemLoaded ? 1 : LoadFilesystem();
		CHECK(iFilesystem, "Failed to load file system")
		int iEngine = m_bEngineLoaded ? 1 : LoadEngine();
		CHECK(iEngine, "Failed to load engine")
		int iMatSys = m_bMatSysLoaded ? 1 : LoadMatSys();
		CHECK(iMatSys, "Failed to load material system")
		int iClient = m_bClientLoaded ? 1 : LoadClient();
		CHECK(iClient, "Failed to load client")

		int iGameUI = m_bGameUILoaded ? 1 : LoadGameUI();
		CHECK(iGameUI, "Failed to load gameui")

		int iParticles = m_bParticlesLoaded ? 1 : LoadParticles();
		CHECK(iParticles, "Failed to load particle system")

		int iMDLCache = m_bMDLCacheLoaded ? 1 : LoadMDLCache();
		CHECK(iMDLCache, "Failed to load MDL cache")

		int iVideoServices = m_bVideoServicesLoaded ? 1 : LoadVideoServices();
		CHECK(iVideoServices, "Failed to load video services")
		
		int iVPhysics = m_bVPhysicsLoaded ? 1 : LoadVPhysics();
		CHECK(iVPhysics, "Failed to load vphysics")

		int iReplay = m_bReplayLoaded ? 1 : LoadReplay();
		CHECK(iReplay, "Failed to load replay")
	}
	while (!isInitializationComplete());

	SDK::Output("TextmodeTF2", std::format("Loaded in {} seconds", SDK::PlatFloatTime()).c_str());

	// Final verification log
	SDK::Output("Core", "Initialization complete. All bytepatches and hooks applied.");

}

void CCore::Unload()
{	
	U::Hooks.Unload();
	U::BytePatches.Unload();

	SDK::Output("TextmodeTF2", "Failed to load");

	ssFailStream << "\nCtrl + C to copy. Logged to TextmodeTF2\\fail_log.txt. \n";
	ssFailStream << "Built @ " __DATE__ ", " __TIME__;
	ssFailStream << "\n\n\n\n";
	std::ofstream file;
	file.open(G::CurrentPath + "\\fail_log.txt", std::ios_base::app);
	file << ssFailStream.str();
	file.close();
	return;
}
