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

	virtual void free(void* p);

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
		Block* blocksHead = nullptr;
		PageHeader* next = nullptr;
		PageHeader* previous = nullptr;
		PageHeader* freeNext = nullptr;
		PageHeader* freePrevious = nullptr;
	};

	// allocs are 64KiB aligned, so restrict alloc size to be less than that and calculate location of page header by rounding down to page alignment
	// pages are page aligned

	PageHeader* head = nullptr;

	PageHeader* pageFreeHead = nullptr;

	size_t pageSize = 0;

	size_t blockSize = 0;

	size_t numBlocksInPage = 0;

	size_t pageHeaderSizeInBlocks = 0;

	PageHeader* allocPage();

	PageHeader* getPage(void* block) const;
	size_t numAllocatedPages = 0;

#ifdef _DEBUG
private:
	
	bool IsBlockFree(PageHeader* page, Block* block) const;
#endif // _DEBUG
};