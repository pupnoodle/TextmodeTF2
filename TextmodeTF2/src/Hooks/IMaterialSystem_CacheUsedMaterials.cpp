#include "../SDK/SDK.h"

MAKE_HOOK(IMaterialSystem_CacheUsedMaterials, U::Memory.GetVFunc(I::MaterialSystem, 67), void, // 67
		  void* rcx)
{
	return;
}
