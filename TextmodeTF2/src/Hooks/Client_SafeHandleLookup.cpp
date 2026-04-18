#include "../Utils/Hooks/Hooks.h"
#include "../SDK/SDK.h"

MAKE_HOOK(Client_SafeHandleLookup, G::Client_SafeHandleLookupAddr, __int64, __int64 rcx)
{
	if (!rcx || rcx == -1)
		return 0;

	__try
	{
		return CALL_ORIGINAL(rcx);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0;
	}
}
