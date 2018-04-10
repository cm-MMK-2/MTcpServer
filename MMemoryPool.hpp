#pragma once
#include <bitset>
#include <deque>

//pool size(DEFAULT_POOL_SIZE * DEFAULT_MAX_BUFFER_SIZE is the memory block size)
#define DEFAULT_POOL_SIZE 2048

typedef struct
{
	char * data;
	std::bitset<DEFAULT_POOL_SIZE> flagset;
} MEMORY_BLOCK;

/*
 * memory management pool
 * make use of large memory blocks instead of small ones
 */
class MMemoryPool
{
public:
	MMemoryPool();

	~MMemoryPool();

	//get a pointer which points to unused DEFAULT_MAX_BUFFER_SIZE size (of bytes) memory
	char* Allocate();

	//return the pointer to block and set its flag as unused
	void Release(char* data);

	//allocate a new area of memory
	void AllocateMemoryBlock();

	//release unused memory block
	void ReleaseMemoryBlock(std::deque<MEMORY_BLOCK>::iterator it);

private:
	//memory block queue
	std::deque<MEMORY_BLOCK> blocks;
};