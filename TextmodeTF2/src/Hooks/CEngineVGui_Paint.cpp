#include "../SDK/SDK.h"

MAKE_HOOK(
	CEngineVGui_Paint,
	U::Memory.FindSignature(
		"engine.dll",
		"4C 8B DC 41 54 41 57 48 81 EC A8 00 00 00 48 8B 05 ? ? ? ? 45 33 E4"),
	void,
	void* rcx,
	int mode)
{
	return;
}
