#include "../SDK/SDK.h"

#include <vector>

MAKE_HOOK(IFileSystem_FindNext, U::Memory.GetVFunc(reinterpret_cast<void*>(G::IFileSystemAddr), 28), const char*,
		  void* rcx, FileFindHandle_t handle)
{
	const char* p;
	do
		p = CALL_ORIGINAL(rcx, handle);
	while (p && SDK::BlacklistFile(p));

	return p;
}

MAKE_HOOK(IFileSystem_FindFirst, U::Memory.GetVFunc(reinterpret_cast<void*>(G::IFileSystemAddr), 27), const char*,
		  void* rcx, const char* pWildCard, FileFindHandle_t* pHandle)
{
	auto p = CALL_ORIGINAL(rcx, pWildCard, pHandle);
	while (p && SDK::BlacklistFile(p))
		p = Hooks::IFileSystem_FindNext::Hook.Call<const char*>(rcx, *pHandle);

	return p;
}

MAKE_HOOK(IFileSystem_AsyncReadMultiple, U::Memory.GetVFunc(reinterpret_cast<void*>(G::IFileSystemAddr), 37), FSAsyncStatus_t,
		  void* rcx, const FileAsyncRequest_t* pRequests, int nRequests, FSAsyncControl_t* phControls)
{
	return CALL_ORIGINAL(rcx, pRequests, nRequests, phControls);
}

MAKE_HOOK(IFileSystem_OpenEx, U::Memory.GetVFunc(reinterpret_cast<void*>(G::IFileSystemAddr), 69), FileHandle_t,
		  void* rcx, const char* pFileName, const char* pOptions, unsigned flags, const char* pathID, char** ppszResolvedFilename)
{
	if (pFileName && SDK::BlacklistFile(pFileName))
		return nullptr;

	return CALL_ORIGINAL(rcx, pFileName, pOptions, flags, pathID, ppszResolvedFilename);
}

MAKE_HOOK(IFileSystem_ReadFileEx, U::Memory.GetVFunc(reinterpret_cast<void*>(G::IFileSystemAddr), 71), int,
		  void* rcx, const char* pFileName, const char* pPath, void** ppBuf, bool bNullTerminate, bool bOptimalAlloc, int nMaxBytes, int nStartingByte, FSAllocFunc_t pfnAlloc)
{
	if (SDK::BlacklistFile(pFileName))
		return 0;

	return CALL_ORIGINAL(rcx, pFileName, pPath, ppBuf, bNullTerminate, bOptimalAlloc, nMaxBytes, nStartingByte, pfnAlloc);
}

MAKE_HOOK(IFileSystem_AddFilesToFileCache, U::Memory.GetVFunc(reinterpret_cast<void*>(G::IFileSystemAddr), 103), void,
		  void* rcx, FileCacheHandle_t cacheId, const char** ppFileNames, int nFileNames, const char* pPathID)
{
	if (!ppFileNames || nFileNames <= 0)
		return;

	std::vector<const char*> vAllowedFiles{};
	vAllowedFiles.reserve(nFileNames);

	for (int i = 0; i < nFileNames; i++)
	{
		const char* pFileName = ppFileNames[i];
		if (!pFileName || SDK::BlacklistFile(pFileName))
			continue;

		vAllowedFiles.push_back(pFileName);
	}

	if (vAllowedFiles.empty())
		return;

	return CALL_ORIGINAL(rcx, cacheId, vAllowedFiles.data(), static_cast<int>(vAllowedFiles.size()), pPathID);
}
