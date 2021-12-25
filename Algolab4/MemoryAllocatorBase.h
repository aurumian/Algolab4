#pragma once

class MemoryAllocatorBase
{
public:
	MemoryAllocatorBase() = default;

	virtual ~MemoryAllocatorBase() = default;

	virtual void init() = 0;

	virtual void destroy() = 0;

	virtual void* alloc(size_t size) = 0;

	virtual void free(void* p) = 0;

#ifdef _DEBUG

	virtual void dumpStat() const = 0;

	virtual void dumpBlocks() const = 0;

private:

	bool isInited;
#endif
};