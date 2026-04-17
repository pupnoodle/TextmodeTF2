#include "BytePatches.h"

#include "../Core/Core.h"
#include <Psapi.h>
#include <string_view>

namespace
{
	bool TryGetModuleBounds(const char* sModule, uintptr_t& uModuleBase, uintptr_t& uModuleEnd)
	{
		const auto hMod = GetModuleHandleA(sModule);
		MODULEINFO moduleInfo = {};
		if (!hMod || !GetModuleInformation(GetCurrentProcess(), hMod, &moduleInfo, sizeof(MODULEINFO)))
			return false;

		uModuleBase = uintptr_t(hMod);
		uModuleEnd = uModuleBase + moduleInfo.SizeOfImage;
		return uModuleEnd >= uModuleBase;
	}

	void ReportModuleInfoFailure(const char* sModule)
	{
		SDK::Output("BytePatches", std::format("Failed to query module info for {}", sModule).c_str());
		U::Core.AppendFailText(std::format("BytePatch::Initialize() failed module info:\n  {}", sModule).c_str());
	}
}

BytePatch::BytePatch(const char* sModule, const char* sSignature, int iOffset, const char* sPatch)
{
	m_sModule = sModule;
	m_sSignature = sSignature;
	m_iOffset = iOffset;

	auto vPatch = U::Memory.PatternToByte(sPatch);
	m_vPatch = vPatch;
	m_iSize = vPatch.size();
	m_vOriginal.resize(m_iSize);
}

BytePatch::BytePatch(const char* sModule, uintptr_t uModuleOffset, const char* sPatch)
{
	m_sModule = sModule;
	m_uModuleOffset = uModuleOffset;
	m_bUseModuleOffset = true;

	auto vPatch = U::Memory.PatternToByte(sPatch);
	m_vPatch = vPatch;
	m_iSize = vPatch.size();
	m_vOriginal.resize(m_iSize);
}

void BytePatch::Write(std::vector<byte>& bytes)
{
	DWORD flNewProtect, flOldProtect;
	VirtualProtect(m_pAddress, m_iSize, PAGE_EXECUTE_READWRITE, &flNewProtect);
	memcpy(m_pAddress, bytes.data(), m_iSize);
	VirtualProtect(m_pAddress, m_iSize, flNewProtect, &flOldProtect);
}

bool BytePatch::Initialize()
{
	if (m_bIsPatched)
		return true;

	uintptr_t uModuleBase = 0;
	uintptr_t uModuleEnd = 0;
	if (!TryGetModuleBounds(m_sModule, uModuleBase, uModuleEnd))
	{
		if (U::Core.m_bTimeout)
			ReportModuleInfoFailure(m_sModule);

		return false;
	}

	return Initialize(uModuleBase, uModuleEnd);
}

bool BytePatch::Initialize(uintptr_t uModuleBase, uintptr_t uModuleEnd)
{
	if (m_bIsPatched)
		return true;

	uintptr_t uAddress = 0;
	if (m_bUseModuleOffset)
	{
		uAddress = uModuleBase + m_uModuleOffset;
	}
	else
	{
		const auto vMatches = U::Memory.FindSignatures(m_sModule, m_sSignature, 2);
		if (vMatches.empty())
		{
			SDK::Output("BytePatches", std::format("Failed to find signature for bytepatch: {} in {}", m_sSignature, m_sModule).c_str());
			U::Core.AppendFailText(std::format("BytePatch::Initialize() failed to initialize:\n  {}\n  {}", m_sModule, m_sSignature).c_str());
			return false;
		}
		if (vMatches.size() != 1)
		{
			SDK::Output("BytePatches", std::format("Ambiguous signature for bytepatch ({} matches): {} in {}", vMatches.size(), m_sSignature, m_sModule).c_str());
			U::Core.AppendFailText(std::format("BytePatch::Initialize() ambiguous signature:\n  {}\n  {}\n  {} matches", m_sModule, m_sSignature, vMatches.size()).c_str());
			return false;
		}

		uAddress = uintptr_t(vMatches[0]) + m_iOffset;
	}
	if (uAddress < uModuleBase || uAddress + m_iSize > uModuleEnd || uAddress + m_iSize < uAddress)
	{
		const auto sLocator = m_bUseModuleOffset
			? std::format("{}+{:#x}", m_sModule, m_uModuleOffset)
			: std::string(m_sSignature);
		SDK::Output("BytePatches", std::format("Refusing out-of-module bytepatch write: {:#x} ({})", uAddress, m_sModule).c_str());
		U::Core.AppendFailText(std::format("BytePatch::Initialize() out-of-range write blocked:\n  {}\n  {}\n  {:#x}", m_sModule, sLocator, uAddress).c_str());
		return false;
	}

	m_pAddress = LPVOID(uAddress);

	DWORD flNewProtect, flOldProtect;
	VirtualProtect(m_pAddress, m_iSize, PAGE_EXECUTE_READWRITE, &flNewProtect);
	memcpy(m_vOriginal.data(), m_pAddress, m_iSize);
	VirtualProtect(m_pAddress, m_iSize, flNewProtect, &flOldProtect);

	Write(m_vPatch);

	if (m_bUseModuleOffset)
		SDK::Output("BytePatches", std::format("Successfully patched {:#x} ('{}'+{:#x})!", uintptr_t(m_pAddress), m_sModule, m_uModuleOffset).c_str());
	else
		SDK::Output("BytePatches", std::format("Successfully patched {:#x} ('{}', '{}')!", uintptr_t(m_pAddress), m_sModule, m_sSignature).c_str());
	return m_bIsPatched = true;
}

void BytePatch::Unload()
{
	if (!m_bIsPatched)
		return;

	Write(m_vOriginal);
	m_bIsPatched = false;
}

bool CBytePatches::Initialize(const char* cModule)
{
	const auto it = m_mPatches.find(cModule);
	if (it == m_mPatches.end() || it->second.empty())
		return true;

	uintptr_t uModuleBase = 0;
	uintptr_t uModuleEnd = 0;
	std::string_view sCurrentModule;
	bool bFail{false};

	for (auto& patch : it->second)
	{
		const std::string_view sPatchModule = patch.GetModule();
		if (sPatchModule != sCurrentModule)
		{
			if (!TryGetModuleBounds(patch.GetModule(), uModuleBase, uModuleEnd))
			{
				if (U::Core.m_bTimeout)
					ReportModuleInfoFailure(patch.GetModule());

				return false;
			}

			sCurrentModule = sPatchModule;
		}

		if (!patch.Initialize(uModuleBase, uModuleEnd))
			bFail = true;
	}

	if (!bFail)
		SDK::Output("BytePatches", std::format("Successfully initialized all byte patches for {}!", cModule).c_str());

	return !bFail;
}

void CBytePatches::Unload()
{
	for (auto [_, vPatches] : m_mPatches)
		for(auto& tPatch : vPatches)
			tPatch.Unload();
}
