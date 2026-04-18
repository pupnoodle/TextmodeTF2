#include "../Utils/Hooks/Hooks.h"
#include "../SDK/SDK.h"

namespace
{
	void ZeroTransform(__m128* matrix)
	{
		if (!matrix)
			return;

		const auto zero = _mm_setzero_ps();
		matrix[0] = zero;
		matrix[1] = zero;
		matrix[2] = zero;
	}
}

MAKE_HOOK(Client_SafeMatrixTransform, G::Client_SafeMatrixTransformAddr, void, __m128* rcx, __m128* rdx, __m128* r8)
{
	if (!rcx || !rdx || !r8)
	{
		ZeroTransform(r8);
		return;
	}

	__try
	{
		CALL_ORIGINAL(rcx, rdx, r8);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		ZeroTransform(r8);
	}
}
