#pragma once

#include "FixedSizeAllocator.h"

#include <iostream>
#include <crtdbg.h>
#include <Windows.h>

static const size_t kDefaultBlockSize = 512U;

static const size_t kMinBlockSize = 64U;

void FixedSizeAllocator::init()
{
	init(kDefaultBlockSize);
}

void FixedSizeAllocator::init(size_t blockSize)
{
	if (blockSize < kMinBlockSize)
	{
		this->blockSize = kMinBlockSize;
	}
	else
	{
		// Round block up size to a mutiple of 8
		const bool remainder = blockSize % 8;
		this->blockSize = remainder ? blockSize + 8 - remainder : blockSize;
	}

	SYSTEM_INFO info;
	GetSystemInfo(&info);
	pageSize = info.dwPageSize;

	_ASSERTE(blockSize < pageSize);

	// alloc a page
	pageFreeHead = allocPage();
}

void FixedSizeAllocator::destroy()
{
	PageHeader* page = head;
	while (page != nullptr)
	{
		PageHeader* toFree = page;
		page = page->next;
		VirtualFree(toFree, 0, MEM_RELEASE);
	}
	head = nullptr;
}

void* FixedSizeAllocator::alloc(size_t size)
{
	_ASSERTE(size < blockSize && "Cannot allocate a memory block larger than blockSize");
	// align size to block size
	size = blockSize;

	// find a page with at least one free block or create a new one
	if (pageFreeHead == nullptr)
	{
		pageFreeHead = allocPage();
	}

	Block* block = pageFreeHead->blocksHead;
	pageFreeHead->blocksHead = block->next;
	
	pageFreeHead->numAllocs += 1;
	if (pageFreeHead->blocksHead == nullptr)
	{
		// Every block was allocated from the page
		pageFreeHead = pageFreeHead->freeNext;
		pageFreeHead->freePrevious = nullptr;
	}

	// todo: memset the block to zeroes ?

	// return address of the newly allocated block
	return block;
}

void FixedSizeAllocator::free(void* p)
{
	_ASSERTE(p != nullptr);

	PageHeader* page = getPage(p);

	Block* block = static_cast<Block*>(p);
	block->next = page->blocksHead;
	page->blocksHead = block;
	page->numAllocs -= 1;

	if (page->numAllocs == numBlocksInPage - 1)
	{
		// return the page to the page free list
		PageHeader* ph = pageFreeHead;
		if (ph == nullptr)
		{
			pageFreeHead = page;
		}
		else
		{
			bool insertAtEnd = false;
			while (ph && ph < page)
			{
				if (ph->freeNext == nullptr)
				{
					insertAtEnd = true;
					break;
				}
				ph = ph->freeNext;
			}
			if (insertAtEnd)
			{ 
				page->freePrevious = ph;
				page->freeNext = nullptr;
				ph->freeNext = page;
			}
			else {
				page->freeNext = ph;
				page->freePrevious = ph->freePrevious;
				ph->freePrevious = page;
				if (ph == pageFreeHead)
				{
					pageFreeHead = page;
				}
			}
		}
	}

	if (page->numAllocs == 0 && numAllocatedPages > 1)
	{
		// Remove it from the page list 
		if (page->previous)
		{
			page->previous->next = page->next;
		}
		if (page->next)
		{
			page->next->previous = page->previous;
		}

		// Remove the page from the page free list
		if (page->freePrevious)
		{
			page->freePrevious->freeNext = page->freeNext;
		}
		if (page->freeNext)
		{
			page->freeNext->freePrevious = page->freePrevious;
		}

		// Return the page back to the system
		VirtualFree(page, 0, MEM_RELEASE);

		--numAllocatedPages;
	}
}

FixedSizeAllocator::PageHeader* FixedSizeAllocator::allocPage()
{
	PageHeader* ph = static_cast<PageHeader*>(VirtualAlloc(nullptr, pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	_ASSERTE(ph && "Could not allocate page from memory");
	ph->numAllocs = 0;

	// Get address of the first block
	pageHeaderSizeInBlocks = sizeof(PageHeader) / blockSize;
	if (sizeof(PageHeader) % blockSize > 0)
	{
		++pageHeaderSizeInBlocks;
	}
	ph->blocksHead = reinterpret_cast<Block*>(reinterpret_cast<UINT8*>(ph) + blockSize * pageHeaderSizeInBlocks);

	// Init the block free list
	numBlocksInPage = pageSize / blockSize - pageHeaderSizeInBlocks;
	Block* block = ph->blocksHead;
	for (size_t i = 0; i < numBlocksInPage - 1; ++i)
	{
		Block* cur = block;
		block = reinterpret_cast<Block*>(reinterpret_cast<UINT8*>(block) + blockSize);
		cur->next = block;
	}

	// Update page free list
	if (head != nullptr)
	{
		ph->next = head;
	}
	head = ph;

	++numAllocatedPages;

	return ph;
}

/** Iterates throught page free list to see if the block belongs to a page in it
  * Returns a pointer to the page if found
*/
FixedSizeAllocator::PageHeader* FixedSizeAllocator::getPage(void* block) const
{
	// Note: assuming allocations are page aligned (or allocation granularity aligned (64 KiB)) 
	// (which they are in our case (committing memory aligns it to SYSTEM_INFO.dwPageSize (4096 Bytes), 
	// reserving (and committing at the same time) aligns to SYSTEM_INFO.dwAllocationGranularity (64KiB)
	// and we use pages 4KiB in size)
	// alternatively we would need to go through all the pages and check that the block is in the page's boundary
	return reinterpret_cast<PageHeader*>((reinterpret_cast<size_t>(block) / pageSize) * pageSize);
}



#ifdef _DEBUG

bool FixedSizeAllocator::IsBlockFree(PageHeader* page, Block* block) const
{
	Block* b = page->blocksHead;
	while (b)
	{
		if (b == block)
			return true;
		b = b->next;
	}
	return false;
}

void FixedSizeAllocator::dumpStat() const
{
	size_t freeCount = 0;
	size_t allocatedCount = 0;

	for (PageHeader* p = head; p; p = p->next) {
		freeCount += numBlocksInPage - p->numAllocs;
		allocatedCount += p->numAllocs;
	}

	std::cout << "FSA DumpStat:" << std::endl;
	std::cout << '\t' << "Free blocks: \t" << numBlocksInPage * numAllocatedPages - allocatedCount << std::endl;
	std::cout << '\t' << "Allocated blocks: \t" << allocatedCount << std::endl;
	std::cout << '\t' << "Allocated pages: \t" << numAllocatedPages << std::endl;
	
}

void FixedSizeAllocator::dumpBlocks() const
{
	size_t count = 0;

	std::cout << "FSA DumpBlocks: \t" << std::endl;

	for (PageHeader* p = head; p; p = p->next) {
		for (Block* b = reinterpret_cast<Block*>(reinterpret_cast<UINT8*>(p) + pageHeaderSizeInBlocks * blockSize); 
			b < reinterpret_cast<Block*>(reinterpret_cast<UINT8*>(p) + pageSize);
			b = reinterpret_cast<Block*>(reinterpret_cast<UINT8*>(b) + blockSize)) {
			if (!IsBlockFree(p, b)) {
				std::cout << "\t" << b << "\t" << blockSize << "\t\t" << std::endl;
				++count;
			}
		}
	}

	std::cout << "Total: \t" << count << " blocks." << std::endl;
}
#endif
