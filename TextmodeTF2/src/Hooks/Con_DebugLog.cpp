#include "../SDK/SDK.h"

namespace
{
	bool should_capture_con_debug_log()
	{
		static const bool enabled = []
		{
			char value[8]{};
			const auto length = GetEnvironmentVariableA("TEXTMODE_CAPTURE_CON_DEBUG_LOG", value, sizeof(value));
			return length > 0 && value[0] != '0';
		}();

		return enabled;
	}
}

MAKE_HOOK(Con_DebugLog, G::Con_DebugLogAddr, void,
		  const char *fmt, ...)
{
	if (!should_capture_con_debug_log())
		return;

	va_list argptr; 
	char data[4096];
    
    va_start(argptr, fmt);
    _vsnprintf_s(data, sizeof(data), _TRUNCATE, fmt, argptr);
    va_end(argptr);

	// Write to our own file
	SDK::OutputFile(CONSOLE_LOG_FILE, data);
}
