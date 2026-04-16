#include "../SDK/SDK.h"

MAKE_HOOK(
	CMumbleSystem_LevelInitPostEntity,
	U::Memory.FindSignature(
		"client.dll",
		"48 83 EC 38 48 83 3D ? ? ? ? 00 75 ? 4C 8D 05 ? ? ? ? 33 D2 B9 1F 00 0F 00 FF 15 ? ? ? ? 48 89 05 ? ? ? ? 48 85 C0 74 ? 45 33 C9"),
	void,
	void* rcx)
{
	return;
}
