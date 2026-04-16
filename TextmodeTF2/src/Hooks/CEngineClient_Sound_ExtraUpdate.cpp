#include "../SDK/SDK.h"

namespace
{
	uintptr_t GetSoundExtraUpdateAddress()
	{
		if (const auto pEngineClient = U::Memory.FindInterface("engine.dll", "VEngineClient013"))
			return reinterpret_cast<uintptr_t>(U::Memory.GetVFunc(pEngineClient, 33));

		return 0;
	}
}

MAKE_HOOK(CEngineClient_Sound_ExtraUpdate, GetSoundExtraUpdateAddress(), void,
		  void* rcx)
{
	return;
}
