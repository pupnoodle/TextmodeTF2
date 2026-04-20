#include "../Utils/Hooks/Hooks.h"
#include "../SDK/SDK.h"

namespace
{
	constexpr size_t kHandleLookupReadBytes = 0x88C;
	constexpr DWORD kReadablePageFlags =
		PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
		PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

	bool HasPageAccess(const void* address, size_t size, DWORD accessFlags)
	{
		if (!address || !size)
			return false;

		uintptr_t current = reinterpret_cast<uintptr_t>(address);
		const uintptr_t end = current + size;
		if (end < current)
			return false;

		while (current < end)
		{
			MEMORY_BASIC_INFORMATION mbi{};
			if (!VirtualQuery(reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi)))
				return false;

			if (mbi.State != MEM_COMMIT)
				return false;

			const DWORD protect = mbi.Protect;
			if ((protect & (PAGE_GUARD | PAGE_NOACCESS)) || !(protect & accessFlags))
				return false;

			const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
			if (regionEnd <= current)
				return false;

			current = regionEnd;
		}

		return true;
	}
}

MAKE_HOOK(Client_SafeHandleLookup, G::Client_SafeHandleLookupAddr, __int64, __int64 rcx)
{
	if (!rcx || rcx == -1 || !HasPageAccess(reinterpret_cast<const void*>(rcx), kHandleLookupReadBytes, kReadablePageFlags))
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
