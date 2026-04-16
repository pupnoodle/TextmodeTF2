#include "Signatures.h"

#include "../Memory/Memory.h"
#include "../../Core/Core.h"
#include <string>
#include <format>

CSignature::CSignature(const char* sDLLName, const char* sSignature, int nOffset, const char* sName)
{
	m_pszDLLName = sDLLName;
	m_pszSignature = sSignature;
	m_nOffset = nOffset;
	m_pszName = sName;

	U::Signatures.AddSignature(this);
}

bool CSignature::Initialize()
{
	m_dwVal = U::Memory.FindSignature(m_pszDLLName, m_pszSignature);
	if (!m_dwVal) {
		U::Core.AppendFailText(std::format("CSignature::Initialize() failed to initialize:\n  {}\n  {}\n  {}", m_pszName, m_pszDLLName, m_pszSignature).c_str());
		return false;
	}

	return m_dwVal += m_nOffset;
}

bool CSignatures::Initialize(const char* sName)
{
	auto pSignature = m_mSignatures[sName];
	return pSignature && pSignature->Initialize();
}

bool CSignatures::Initialize()
{
	bool bFail{ false };
	for (auto [_, pSignature] : m_mSignatures)
	{
		if (!pSignature || !pSignature->Initialize())
			bFail = true;
	}
	return !bFail;
}