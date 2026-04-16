#pragma once
#include "../../../Utils/Interfaces/Interfaces.h"
#include "../../../Utils/Signatures/Signatures.h"

class IBaseInterface
{
public:
	virtual	~IBaseInterface() {}
};

typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);