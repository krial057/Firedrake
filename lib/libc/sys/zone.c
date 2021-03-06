//
//  sys/zone.c
//  libc
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "../math.h"
#include "../string.h"
#include "../stdio.h"
#include "mman.h"
#include "vm.h"
#include "zone.h"

#define assert(e) ((void)0)

#define kHeapAllocationExtraPadding 16 // Extra padding bytes per allocation

#define kZoneAllocationTypeFree   0
#define kZoneAllocationTypeUsed   1
#define kZoneAllocationTypeUnused 2

typedef struct
{
	uint8_t type;
	uint8_t size;
	uint16_t offset;
} zone_tiny_allocation_t;

typedef struct
{
	uint8_t type;
	size_t size;
	uintptr_t pointer;
} zone_allocation_t;

zone_t *_zone_create(unsigned int flags)
{
	zone_t *zone = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	if(zone != MAP_FAILED)
	{
		zone->flags = flags;
		zone->lock = SPINLOCK_INIT;

		zone->firstZone = NULL;
	}

	return zone;
}

void _zone__destroy(zone_t *zone)
{
	heap_zone_t *tzone = zone->firstZone;
	while(tzone)
	{
		heap_zone_t *next = tzone->next;

		munmap(tzone, tzone->pages * VM_PAGE_SIZE);

		tzone = next;
	}

	munmap(zone, 4096);
}


// ------------------------------
// Zone management
// ------------------------------

static inline heap_zone_type_t __heap_zoneTypeForSize(size_t size)
{
	if(size > 2048)
		return heap_zone_typeLarge;

	if(size > 256)
		return heap_zone_typeMedium;

	if(size > 64)
		return heap_zone_typeSmall;

	return heap_zone_typeTiny;
}

heap_zone_t *__heap_createZoneForSize(zone_t *heap, size_t size)
{
	heap_zone_type_t type = __heap_zoneTypeForSize(size);
	size_t pages = 0;

	switch(type)
	{
		case heap_zone_typeTiny:
			pages = 1;
			break;

		case heap_zone_typeSmall:
			pages = 5;
			break;

		case heap_zone_typeMedium:
			pages = 20;
			break;

		case heap_zone_typeLarge:
			pages = VM_PAGE_COUNT(size);
			break;
	}

	heap_zone_t *zone = mmap(NULL, (pages + 1) * VM_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	assert(zone != MAP_FAILED);

	uintptr_t address = (uintptr_t)zone;
	size_t allocationSize = (type == heap_zone_typeTiny) ? sizeof(zone_tiny_allocation_t) : sizeof(zone_allocation_t);

	zone->type = type;
	zone->changes = 0;

	zone->begin = address + VM_PAGE_SIZE;
	zone->end   = address + (pages * VM_PAGE_SIZE) + VM_PAGE_SIZE;

	zone->pages = pages + 1;
	zone->freeSize = (pages * VM_PAGE_SIZE);

	zone->maxAllocations  = (VM_PAGE_SIZE - sizeof(heap_zone_t)) / allocationSize;
	zone->allocations     = 0;
	zone->freeAllocations = 0;

	zone->firstAllocation = (void *)(address + sizeof(heap_zone_t));
	zone->lastAllocation  = (void *)(address + (zone->maxAllocations * allocationSize));

	zone->prev = NULL;
	zone->next = heap->firstZone;

	if(heap->firstZone)
		heap->firstZone->prev = zone;

	heap->firstZone = zone;

	if(type == heap_zone_typeTiny)
	{
		size_t sizeLeft = zone->freeSize;
		uint16_t offset = 0;

		zone_tiny_allocation_t *allocation = zone->firstAllocation;

		for(; allocation < (zone_tiny_allocation_t *)zone->lastAllocation; allocation ++)
		{
			if(sizeLeft > 0)
			{
				allocation->size = MIN(sizeLeft, UINT8_MAX);
				allocation->offset = offset;
				allocation->type = kZoneAllocationTypeFree;

				offset += allocation->size;
				sizeLeft -= allocation->size;

				zone->allocations ++;
				zone->freeAllocations ++;
			}
			else
			{
				allocation->type = kZoneAllocationTypeUnused;
				allocation->offset = UINT16_MAX;
			}
		}
	}
	else
	{
		zone_allocation_t *allocation = zone->firstAllocation;

		zone->allocations     = 1;
		zone->freeAllocations = 1;

		allocation->type = kZoneAllocationTypeFree;
		allocation->size = zone->freeSize;
		allocation->pointer = zone->begin;

		for(allocation ++; allocation < (zone_allocation_t *)zone->lastAllocation; allocation ++)
		{
			allocation->pointer = 0;
			allocation->type = kZoneAllocationTypeUnused;
		}
	}

	return zone;
}

void __heap_destroyZone(zone_t *heap, heap_zone_t *zone)
{
	// Fix the linked list
	if(zone->prev)
		zone->prev->next = zone->next;

	if(zone->next)
		zone->next->prev = zone->prev;

	if(zone == heap->firstZone)
		heap->firstZone = zone->next;

	// Get rid of the zone
	munmap(zone, zone->pages * VM_PAGE_SIZE);
}


heap_zone_t *__heap_zoneForSize(zone_t *heap, size_t size, void **outAllocation)
{
	heap_zone_type_t type = __heap_zoneTypeForSize(size);
	heap_zone_t *zone = heap->firstZone;

	if(type != heap_zone_typeLarge)
	{	
		size_t padding = (heap->flags & kZoneFlagAligned) ? size % 4 : 0;
		size_t required = size + padding + kHeapAllocationExtraPadding;
		
		while(zone)
		{
			if(zone->type == type && zone->freeSize >= size && zone->allocations < zone->maxAllocations)
			{
				if(zone->type == heap_zone_typeTiny)
				{
					zone_tiny_allocation_t *allocation = zone->firstAllocation;
					for(; allocation < (zone_tiny_allocation_t *)zone->lastAllocation; allocation ++)
					{
						if(allocation->type == kZoneAllocationTypeFree && allocation->size >= required)
						{
							if(outAllocation)
								*outAllocation = allocation;

							return zone;
						}
					}
				}
				else
				{
					zone_allocation_t *allocation = zone->firstAllocation;
					for(; allocation < (zone_allocation_t *)zone->lastAllocation; allocation ++)
					{
						if(allocation->type == kZoneAllocationTypeFree && allocation->size >= required)
						{
							if(outAllocation)
								*outAllocation = allocation;
							
							return zone;
						}
					}
				}
			}

			zone = zone->next;
		}
	}

	// We couldn't find a zone with enough free space, so let's create a fresh one
	zone = __heap_createZoneForSize(heap, size);

	if(outAllocation)
		*outAllocation = zone->firstAllocation;

	return zone;
}

static inline void *__heap_zoneFindAllocationWithPointer(heap_zone_t *zone, uintptr_t pointer)
{
	if(zone->type == heap_zone_typeTiny)
	{
		zone_tiny_allocation_t *allocation = zone->firstAllocation;
		for(; allocation < (zone_tiny_allocation_t *)zone->lastAllocation; allocation ++)
		{
			if(pointer == zone->begin + allocation->offset && allocation->type == kZoneAllocationTypeUsed)
			{
				return allocation;
			}
		}
	}
	else
	{
		zone_allocation_t *allocation = zone->firstAllocation;
		for(; allocation < (zone_allocation_t *)zone->lastAllocation; allocation ++)
		{
			if(allocation->pointer == pointer && allocation->type == kZoneAllocationTypeUsed)
			{
				return allocation;
			}
		}
	}

	return NULL;
}

heap_zone_t *__heap_findZoneWithAllocation(zone_t *heap, uintptr_t pointer, void **outAllocation)
{
	heap_zone_t *zone = heap->firstZone;
	while(zone)
	{
		if(pointer >= zone->begin && pointer < zone->end)
		{
			void *allocation = __heap_zoneFindAllocationWithPointer(zone, pointer);
			if(allocation)
			{
				if(outAllocation)
					*outAllocation = allocation;

				return zone;
			}

			break;
		}

		zone = zone->next;
	}

	return NULL;
}


size_t __heap_sizeForPointer(zone_t *heap, uintptr_t ptr)
{
	spinlock_lock(&heap->lock);

	void *allocation = NULL;
	heap_zone_t *zone = __heap_findZoneWithAllocation(heap, ptr, &allocation);

	if(zone && allocation)
	{
		size_t size;

		if(zone->type == heap_zone_typeTiny)
		{
			zone_tiny_allocation_t *temp = allocation;
			size = temp->size;
		}
		else
		{
			zone_allocation_t *temp = allocation;
			size = temp->size;
		}
		
		spinlock_unlock(&heap->lock);	
		return size;
	}

	spinlock_unlock(&heap->lock);	
	return 0;
}

static inline void *__heap_zoneGetUnusedAllocation(heap_zone_t *zone)
{
	if(zone->type == heap_zone_typeTiny)
	{
		zone_tiny_allocation_t *allocation = zone->firstAllocation;
		for(; allocation < (zone_tiny_allocation_t *)zone->lastAllocation; allocation ++)
		{
			if(allocation->type == kZoneAllocationTypeUnused)
				return allocation;
		}
	}	
	else
	{
		zone_allocation_t *allocation = zone->firstAllocation;
		for(; allocation < (zone_allocation_t *)zone->lastAllocation; allocation ++)
		{
			if(allocation->type == kZoneAllocationTypeUnused)
				return allocation;
		}
	}

	return NULL;
}

static inline void __heap_zoneDefragment(heap_zone_t *zone)
{
	size_t changeThreshold = (zone->type == heap_zone_typeTiny) ? 100 : 20;

	if(zone->changes >= changeThreshold && zone->freeAllocations >= 2)
	{
		if(zone->type == heap_zone_typeTiny)
		{
			zone_tiny_allocation_t *allocation = zone->firstAllocation;
			for(; allocation < (zone_tiny_allocation_t *)zone->lastAllocation; allocation ++)
			{
				while(allocation->type == kZoneAllocationTypeFree)
				{
					zone_tiny_allocation_t *next = __heap_zoneFindAllocationWithPointer(zone, zone->begin + allocation->offset + allocation->size);
					if(!next || next->type != kZoneAllocationTypeFree)
						break;

					unsigned int size = allocation->size;
					if(size + next->size > UINT8_MAX)
						break;

					allocation->size += next->size;

					next->type = kZoneAllocationTypeUnused;
					next->offset = 0;

					zone->allocations --;
					zone->freeAllocations --;
				}
			}
		}
		else
		{
			zone_allocation_t *allocation = zone->firstAllocation;
			for(; allocation < (zone_allocation_t *)zone->lastAllocation; allocation ++)
			{
				while(allocation->type == kZoneAllocationTypeFree)
				{
					zone_allocation_t *next = __heap_zoneFindAllocationWithPointer(zone, allocation->pointer + allocation->size);
					if(!next || next->type != kZoneAllocationTypeFree)
						break;

					allocation->size += next->size;

					next->type = kZoneAllocationTypeUnused;
					next->pointer = 0;

					zone->allocations --;
					zone->freeAllocations --;
				}
			}
		}

		zone->changes = 0;
	}
}

// ------------------------------
// Allocation management
// ------------------------------

static inline void *__heap_useAllocation(zone_t *heap, heap_zone_t *zone, void *allocationPtr, size_t size)
{
	size_t padding = (heap->flags & kZoneFlagAligned) ? size % 4 : 0;
	size_t required = size + padding + kHeapAllocationExtraPadding;

	if(zone->type == heap_zone_typeTiny)
	{
		zone_tiny_allocation_t *allocation = allocationPtr;
		allocation->type = kZoneAllocationTypeUsed;

		if(allocation->size > required)
		{
			zone_tiny_allocation_t *unused = __heap_zoneGetUnusedAllocation(zone);
			if(unused)
			{
				unused->size = allocation->size - required;
				unused->offset = allocation->offset + required;
				unused->type = kZoneAllocationTypeFree;

				allocation->size = required;

				zone->freeAllocations ++;
				zone->allocations ++;
			}
		}

		zone->freeAllocations --;
		return (void *)(zone->begin + allocation->offset);
	}
	else
	{
		zone_allocation_t *allocation = allocationPtr;
		allocation->type = kZoneAllocationTypeUsed;

		if(allocation->size > required)
		{
			zone_allocation_t *unused = __heap_zoneGetUnusedAllocation(zone);
			if(unused)
			{
				unused->size = allocation->size - required;
				unused->pointer = allocation->pointer + required;
				unused->type = kZoneAllocationTypeFree;

				allocation->size = required;

				zone->freeAllocations ++;
				zone->allocations ++;
			}
		}
		
		zone->freeAllocations --;
		return (void *)(allocation->pointer);
	}

	return NULL;
}

static inline void __heap_freeAllocation(heap_zone_t *zone, void *allocationPtr)
{
	if(zone->type == heap_zone_typeTiny)
	{
		zone_tiny_allocation_t *allocation = allocationPtr;

		allocation->type = kZoneAllocationTypeFree;
		zone->freeSize += allocation->size;
	}
	else
	{
		zone_allocation_t *allocation = allocationPtr;

		allocation->type = kZoneAllocationTypeFree;
		zone->freeSize += allocation->size;
	}

	zone->freeAllocations ++;
	zone->changes ++;
}

//

void *_zone_alloc(zone_t *heap, size_t size)
{
	assert(heap);
	spinlock_lock(&heap->lock);

	heap_zone_t *zone;
	void *allocation = NULL;
	void *pointer = NULL;

	zone = __heap_zoneForSize(heap, size, &allocation);
	assert(zone);

	pointer = __heap_useAllocation(heap, zone, allocation, size);
	assert(pointer);

	spinlock_unlock(&heap->lock);

	if(heap->flags & kZoneFlagSecure)
		memset(pointer, 0, size);

	return pointer;
}

void _zone_free(zone_t *heap, void *ptr)
{
	assert(heap);
	spinlock_lock(&heap->lock);

	void *allocationPtr = NULL;
	heap_zone_t *zone = __heap_findZoneWithAllocation(heap, (uintptr_t)ptr, &allocationPtr);

	if(zone->allocations == zone->freeAllocations + 1)
	{
		__heap_destroyZone(heap, zone);

		spinlock_unlock(&heap->lock);
		return;
	}
	
	__heap_freeAllocation(zone, allocationPtr);
	__heap_zoneDefragment(zone);

	spinlock_unlock(&heap->lock);
}
