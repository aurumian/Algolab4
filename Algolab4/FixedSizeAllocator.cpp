#pragma once

#include "FixedSizeAllocator.h"

#include <Windows.h>

static const size_t kDefaultBlockSize = 512U;

static const size_t kMinBlockSize = 8U;

void FixedSizeAllocator::init()
{
	init(kDefaultBlockSize);
}

void FixedSizeAllocator::init(size_t blockSize)
{
	// todo: blockSize must be a power of 2
	this->blockSize = blockSize < kMinBlockSize ? kMinBlockSize : blockSize;

	// alloc a page
	allocPage();


}

void FixedSizeAllocator::destroy()
{
}

void* FixedSizeAllocator::alloc(size_t size)
{
	// align size to block size

	// find a page with at least one free block

	// return address of the block

	return nullptr;
}

void* FixedSizeAllocator::free(void* p)
{
	return nullptr;
}

FixedSizeAllocator::PageHeader* FixedSizeAllocator::allocPage()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	const DWORD pageSize = info.dwPageSize;

	// todo: check ph for null
	PageHeader* ph = static_cast<PageHeader*>(VirtualAlloc(nullptr, pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	ph->numAllocs = 0;

	// Get address of the first slot
	size_t pageHeaderSizeInBlocks = sizeof(PageHeader) / blockSize;
	if (sizeof(PageHeader) % blockSize > 0)
	{
		++pageHeaderSizeInBlocks;
	}
	ph->slotsHead = reinterpret_cast<Block*>(reinterpret_cast<size_t>(ph) + blockSize * pageHeaderSizeInBlocks);

	// Init the block free list
	const size_t numBlocksInPage = pageSize / blockSize - pageHeaderSizeInBlocks;
	Block* block = ph->slotsHead;
	for (size_t i = 0; i < numBlocksInPage - 1; ++i)
	{
		Block* cur = block;
		block = reinterpret_cast<Block*>(reinterpret_cast<size_t>(block) + blockSize);
		cur->next = block;
	}

	// Update page free list
	// todo: insert with sorting in mind
	if (head != nullptr)
	{
		ph->next = head;
	}
	head = ph;

	return ph;
}

#ifdef _DEBUG

void FixedSizeAllocator::dumpStat() const
{
}

void FixedSizeAllocator::dumpBlocks() const
{
}
#endif
