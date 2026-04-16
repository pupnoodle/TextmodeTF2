#include "../SDK/SDK.h"

MAKE_HOOK(
	C_SoundscapeSystem_Update,
	U::Memory.FindSignature(
		"client.dll",
		"48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 41 56 41 57 48 8D 68 ? 48 81 EC 00 01 00 00 83 B9 10 01 00 00 00 48 8B F1 0F 29 70 D8 44 0F 29 48 A8 44 0F 28 C9 0F 8C ? ? ? ? E8 ? ? ? ? 48 85 C0 0F 84"),
	void,
	void* rcx,
	float frametime)
{
	return;
}
