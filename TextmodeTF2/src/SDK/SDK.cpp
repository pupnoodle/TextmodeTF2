#include "SDK.h"
#include <string_view>
#include <unordered_set>

void SDK::Output(const char* cFunction, const char* cLog, bool bLogFile, int iMessageBox)
{
#ifdef _DEBUG
	if (!GetConsoleWindow())
	{
		AllocConsole();
		FILE* pFile;
		freopen_s(&pFile, "CONOUT$", "w", stdout);
	}
#endif

	if (cLog)
	{
		std::string sLog = std::format("[{}] {}", cFunction, cLog);
#ifdef _DEBUG
		printf("%s\n", sLog.c_str());
		fflush(stdout);
#endif

		if (bLogFile)
			OutputFile(TEXTMODE_LOG_FILE, (sLog + "\n").c_str());
		if (iMessageBox != -1)
			MessageBox(nullptr, cLog, cFunction, iMessageBox);
	}
	else
	{
#ifdef _DEBUG
		printf("%s\n", cFunction);
		fflush(stdout);
#endif

		if (bLogFile)
			OutputFile(TEXTMODE_LOG_FILE, std::format("{}\n", cFunction).c_str());
		if (iMessageBox != -1)
			MessageBox(nullptr, "", cFunction, iMessageBox);
	}
}

void SDK::OutputFile(const char* cOutputFileName, const char* cMsg)
{
	std::string sPath = G::AppdataPath.empty() ? G::CurrentPath + "\\" : G::AppdataPath;
	if (sPath == "\\" || sPath.empty())
	{
		char szPath[MAX_PATH];
		if (GetModuleFileNameA(NULL, szPath, MAX_PATH))
		{
			std::string sModulePath(szPath);
			sPath = sModulePath.substr(0, sModulePath.find_last_of("\\/")) + "\\";
		}
	}

	try
	{
		std::ofstream file;
		file.open(sPath + cOutputFileName, std::ios::app);
		if (file.is_open())
		{
			file << cMsg;
			file.close();
		}
		
		if (!file.is_open()) {
			std::ofstream backupFile;
			backupFile.open("C:\\TextmodeLog_Backup.log", std::ios::app);
			if (backupFile.is_open()) {
				backupFile << cMsg;
				backupFile.close();
			}
		}
	}
	catch (...) {}
}

bool SDK::BlacklistFile(const char* cFileName)
{
	if (!cFileName)
		return false;

	using namespace std::literals;

	static const std::unordered_set<std::string_view> blacklist{
		".ani"sv, ".wav"sv, ".mp3"sv, ".vvd"sv, ".vtx"sv, ".vtf"sv, ".vfe"sv, ".cache"sv,
		".jpg"sv, ".png"sv, ".tga"sv, ".dds"sv, ".vmt"sv,
		".phy"sv, ".dem"sv, ".vcd"sv, ".ain"sv, ".lst"sv, ".pcf"sv
	};

	const std::string_view fileName{ cFileName };

	const bool isSoundscapeManifest{
		fileName == "scripts/soundscapes_manifest.txt"sv || fileName == "scripts\\soundscapes_manifest.txt"sv
	};
	const bool isSoundscapeScript{
		(fileName.starts_with("scripts/soundscapes_"sv) || fileName.starts_with("scripts\\soundscapes_"sv)) &&
		fileName.ends_with(".txt"sv)
	};
	const bool isReplayPath{
		fileName.starts_with("replay/"sv) || fileName.starts_with("replay\\"sv) ||
		fileName.starts_with("resource/replay/"sv) || fileName.starts_with("resource\\replay\\"sv) ||
		fileName.starts_with("materials/vgui/replay/"sv) || fileName.starts_with("materials\\vgui\\replay\\"sv) ||
		fileName.starts_with("media/"sv) || fileName.starts_with("media\\"sv) ||
		fileName.starts_with("videos/"sv) || fileName.starts_with("videos\\"sv)
	};

	// The engine/server still expect the manifest + script text to exist even when
	// the client soundscape system itself is stubbed out.
	if (isSoundscapeManifest || isSoundscapeScript)
		return false;

	if (isReplayPath)
		return true;

	if (fileName.starts_with("materials/console/"sv) || fileName.starts_with("debug/"sv))
		return false;

	if (fileName.starts_with("sprites/"sv))
		return true;

	const auto dot = fileName.rfind('.');
	if (dot == std::string_view::npos)
		return false;
	const auto ext = fileName.substr(dot);

	const bool is_collision_relevant_model_asset{
		ext == ".mdl"sv || ext == ".phy"sv || ext == ".vvd"sv || ext == ".vtx"sv || ext == ".ani"sv
	};

	const bool is_essential_model_asset{
		fileName.find("player/"sv) != std::string_view::npos ||
		fileName.find("player\\"sv) != std::string_view::npos ||
		fileName.find("models/player/"sv) != std::string_view::npos ||
		fileName.find("models\\player\\"sv) != std::string_view::npos ||
		fileName.find("bots/"sv) != std::string_view::npos ||
		fileName.find("bots\\"sv) != std::string_view::npos ||
		fileName.find("models/bots/"sv) != std::string_view::npos ||
		fileName.find("models\\bots\\"sv) != std::string_view::npos ||
		fileName.find("buildables/"sv) != std::string_view::npos ||
		fileName.find("buildables\\"sv) != std::string_view::npos ||
		fileName.find("weapons/"sv) != std::string_view::npos ||
		fileName.find("weapons\\"sv) != std::string_view::npos ||
		fileName.find("empty.mdl"sv) != std::string_view::npos ||
		fileName.find("error.mdl"sv) != std::string_view::npos
	};

	// this probably made catbots miss
	if (is_essential_model_asset && is_collision_relevant_model_asset)
		return false;

	// NEVER block .bsp files - they are essential for map loading
	if (ext == ".bsp"sv)
		return false;

	// NEVER block .nav files - navengine needs it obviosuly lol
	if (ext == ".nav"sv)
		return false;

	// Block all particle effects during map load
	if (ext == ".pcf"sv)
		return true;

	// Block all soundscapes during map load
	if (fileName.find("soundscape"sv) != std::string_view::npos && ext != ".txt"sv)
		return true;

	// Block detail sprites
	if (fileName.find("detail"sv) != std::string_view::npos)
		return true;

	// Block skybox materials during map load
	if (fileName.find("skybox"sv) != std::string_view::npos)
		return true;

	// Block all ambient sounds
	if (fileName.find("ambient"sv) != std::string_view::npos)
		return true;

	if (ext == ".vmt"sv)
	{
		// Only allow essential UI materials
		if (fileName.find("hud"sv) == std::string_view::npos && fileName.find("vgui"sv) == std::string_view::npos && fileName.find("console"sv) == std::string_view::npos)
			return true;

		/* Not loading it causes extreme console spam */
		if (fileName.find("corner"sv) != std::string_view::npos)
			return false;
		/* minor console spam */
		if (fileName.find("hud"sv) != std::string_view::npos || fileName.find("vgui"sv) != std::string_view::npos)
			return false;

		return true;
	}

	if (fileName.find("sound.cache"sv) != std::string_view::npos || fileName.find("tf2_sound"sv) != std::string_view::npos || fileName.find("game_sounds"sv) != std::string_view::npos)
		return false;
	if (fileName.starts_with("sound/player/footsteps"sv))
		return false;
	if (fileName.starts_with("/decal"sv))
		return true;

	return blacklist.contains(ext);
}

double SDK::PlatFloatTime()
{
	static auto Plat_FloatTime = U::Memory.GetModuleExport<double(*)()>("tier0.dll", "Plat_FloatTime");
	return Plat_FloatTime();
}
