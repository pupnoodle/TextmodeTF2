#include "../SDK/SDK.h"

MAKE_HOOK(
	C_SoundscapeSystem_UpdateAudioParams,
	U::Memory.FindSignature(
		"client.dll",
		"48 89 5C 24 08 57 48 83 EC 20 8B 41 68 48 8B D9 39 05 ? ? ? ? 75 ? 8B 41 70 39 05 ? ? ? ? 0F 84 ? ? ? ? 48 8B D3 48 8D 0D ? ? ? ? E8 ? ? ? ? C7 05 ? ? ? ? FF FF FF FF"),
	void,
	void* rcx,
	void* pAudioParams)
{
	return;
}
