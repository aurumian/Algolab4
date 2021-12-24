#pragma once

#include "MemoryAllocatorBase.h"

class FixedSizeAllocator : public MemoryAllocatorBase
{
public:
	FixedSizeAllocator() {};

	~FixedSizeAllocator() {};

	virtual void init() override;

	void init(size_t blockSize);

	virtual void destroy();

	virtual void* alloc(size_t size);

	virtual void* free(void* p);

#ifdef _DEBUG

	virtual void dumpStat() const override;

	virtual void dumpBlocks() const override;

#endif

private:
	struct Block
	{
		Block* next;
	};
	struct PageHeader
	{
		size_t numAllocs = 0;
		Block* slotsHead = nullptr;
		PageHeader* next = nullptr;
	};

	PageHeader* head = nullptr;

	size_t blockSize = 0;

	PageHeader* allocPage();
};