#include "../Utils/Hooks/Hooks.h"
#include "../SDK/SDK.h"

namespace
{
	constexpr size_t kMatrixTransformBytes = sizeof(__m128) * 3;
	constexpr DWORD kReadablePageFlags =
		PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
		PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
	constexpr DWORD kWritablePageFlags =
		PAGE_READWRITE | PAGE_WRITECOPY |
		PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

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

	bool IsReadableMatrix(const __m128* matrix)
	{
		return HasPageAccess(matrix, kMatrixTransformBytes, kReadablePageFlags);
	}

	bool IsWritableMatrix(__m128* matrix)
	{
		return HasPageAccess(matrix, kMatrixTransformBytes, kWritablePageFlags);
	}

	void ZeroTransform(__m128* matrix)
	{
		if (!IsWritableMatrix(matrix))
			return;

		const auto zero = _mm_setzero_ps();
		matrix[0] = zero;
		matrix[1] = zero;
		matrix[2] = zero;
	}
}

MAKE_HOOK(Client_SafeMatrixTransform, G::Client_SafeMatrixTransformAddr, void, __m128* rcx, __m128* rdx, __m128* r8)
{
	if (!IsReadableMatrix(rcx) || !IsReadableMatrix(rdx))
	{
		ZeroTransform(r8);
		return;
	}

	if (!IsWritableMatrix(r8))
		return;

	__try
	{
		CALL_ORIGINAL(rcx, rdx, r8);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		ZeroTransform(r8);
	}
}
