#include "../SDK/SDK.h"

MAKE_HOOK(CVideoModeCommon_SetupStartupGraphic, G::CVideoModeCommon_SetupStartupGraphicAddr, void*,
		  void* rcx)
{
	return nullptr;
}