#include "../SDK/SDK.h"

MAKE_HOOK(
	C_SoundscapeSystem_Init,
	U::Memory.FindSignature(
		"client.dll",
		"48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 48 8B E9 C7 81 0C 01 00 00 00 00 00 00 E8 ? ? ? ? 33 FF 48 85 C0 74 ? 40 38 38 74 ? 48 8B D0 48 8D 0D"),
	bool,
	void* rcx)
{
	return true;
}
