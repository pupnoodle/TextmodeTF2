#include "../SDK/SDK.h"

MAKE_HOOK(Host_IsSecureServerAllowed, G::Host_IsSecureServerAllowedAddr, bool)
{
	return *reinterpret_cast<bool*>(U::Memory.RelToAbs(G::g_bAllowSecureServersAddr)) = true;
}