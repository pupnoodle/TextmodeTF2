#include "../SDK/SDK.h"

namespace
{
	bool ShouldSkipModelData(IMDLCache* pCache, MDLHandle_t handle, MDLCacheDataType_t type)
	{
		if (!pCache)
			return false;

		const char* pModelName = pCache->GetModelName(handle);
		if (pModelName && SDK::BlacklistFile(pModelName))
			return true;

		switch (type)
		{
		case MDLCACHE_STUDIOHWDATA:
		case MDLCACHE_VERTEXES:
			return true;
		default:
			return false;
		}
	}
}

MAKE_HOOK(IMDLCache_ProcessDataIntoCache, U::Memory.GetVFunc(reinterpret_cast<void*>(G::IMDLCache), 33), bool,
		  void* rcx, MDLHandle_t handle, MDLCacheDataType_t type, void* pData, int nDataSize, bool bAsync)
{
	if (ShouldSkipModelData(reinterpret_cast<IMDLCache*>(rcx), handle, type))
		return true;

	return CALL_ORIGINAL(rcx, handle, type, pData, nDataSize, bAsync);
}
