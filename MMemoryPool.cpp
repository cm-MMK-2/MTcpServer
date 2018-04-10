#include "MMemoryPool.hpp"
#include "MClientSession.hpp"
//#include <assert.h>

MMemoryPool::MMemoryPool()
{
	//allocate a block on start up, you can delete this if you want
	AllocateMemoryBlock();
}

MMemoryPool::~MMemoryPool()
{
	//dispose
	for (auto& block : blocks)
	{
		free(block.data);
	}
}

char* MMemoryPool::Allocate()
{
	for (auto& block : blocks)
	{
		//this block is not full
		if (!block.flagset.all())
		{
			for (int i = 0; i < DEFAULT_POOL_SIZE; i++)
			{
				if (!block.flagset.test(i))//flag is not set
				{
					block.flagset.set(i);
					return block.data + i * DEFAULT_MAX_BUFFER_SIZE;//return available data address
				}
			}
		}
	}
	//all blocks are full
	AllocateMemoryBlock();
	blocks.back().flagset.set(0);
	return blocks.back().data;
}

void MMemoryPool::Release(char* data)
{
	for (auto it = blocks.begin(); it != blocks.end(); ++it)
	{
		//data's address is inside this block's area
		if ((intptr_t)data >= (intptr_t)it->data && (intptr_t)data < (intptr_t)it->data + DEFAULT_POOL_SIZE * DEFAULT_MAX_BUFFER_SIZE)
		{
			//test
			//assert(((intptr_t)data - (intptr_t)it->data) % DEFAULT_MAX_BUFFER_SIZE == 0);

			//reset flag
			it->flagset.reset(((intptr_t)data - (intptr_t)it->data)/ DEFAULT_MAX_BUFFER_SIZE);

			//do not release all blocks
			if (blocks.size() > 1 && it->flagset.none())
			{
				ReleaseMemoryBlock(it);
			}
			break;
		}
	}

}

void MMemoryPool::AllocateMemoryBlock()
{
	MEMORY_BLOCK block;
	block.data = (char*)malloc(DEFAULT_MAX_BUFFER_SIZE * DEFAULT_POOL_SIZE);
	//according to the docuement, bitset will be initialized with all bits are 0
	//block.flagset.reset();
	//copy into deque
	blocks.push_back(block);
}

void MMemoryPool::ReleaseMemoryBlock(std::deque<MEMORY_BLOCK>::iterator it)
{
	free(it->data);
	blocks.erase(it);
}