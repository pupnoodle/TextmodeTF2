#include "../SDK/SDK.h"

MAKE_HOOK(
	CMumbleSystem_PostRender,
	U::Memory.FindSignature(
		"client.dll",
		"4C 8B DC 49 89 7B 20 55 49 8D AB D8 FC FF FF 48 81 EC 20 04 00 00 48 8B F9 48 8B 0D ? ? ? ? 48 85 C9 0F 84 ? ? ? ? 48 8B 05 ? ? ? ? 83 78 58 00 0F 84 ? ? ? ? 83 39 02 49 89 5B"),
	void,
	void* rcx)
{
	return;
}
