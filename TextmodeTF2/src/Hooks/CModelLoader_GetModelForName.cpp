#include "../Utils/Hooks/Hooks.h"
#include "../SDK/SDK.h"

// model_t *CModelLoader::GetModelForName( const char *name, REFERENCETYPE referencetype )
MAKE_HOOK(CModelLoader_GetModelForName, G::CModelLoader_GetModelForNameAddr, void*, void* rcx, const char* name, int referencetype)
{
	if (SDK::BlacklistFile(name))
	{
		return CALL_ORIGINAL(rcx, "models/empty.mdl", referencetype);
	}

	// this made catbots miss
	return CALL_ORIGINAL(rcx, name, referencetype);
}
