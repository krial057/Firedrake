//
//  iostore.h
//  Firedrake
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

#include <memory/memory.h>
#include <container/array.h>
#include <system/syslog.h>
#include <system/helper.h>
#include <libc/string.h>
#include "iostore.h"
#include "iostubs.h"
#include "iomodule.h"

//static spinlock_t __io_storeLock = SPINLOCK_INIT;
static atree_t *__io_storeLibraries = NULL;
static spinlock_t __io_storeLock = SPINLOCK_INIT;

static io_library_t *__io_libio = NULL;
static io_library_t *__io_libkernel = NULL;

elf_sym_t *io_storeLookupSymbol(io_library_t *library, uint32_t symNum, io_library_t **outLib)
{
	elf_sym_t *lookup = library->symtab + symNum;
	const char *name  = library->strtab + lookup->st_name;

	if(ELF32_ST_BIND(lookup->st_info) == STB_LOCAL)
	{
		*outLib = library;
		return lookup;
	}
	else
	{
		uint32_t hash = elf_hash(name);
		elf_sym_t *symbol;

		// Try to find the symbol in the kernel
		symbol = io_findKernelSymbol(name);
		if(symbol)
		{
			*outLib = io_kernelLibraryStub();
			return symbol;
		}

		// Search through the dependencies
		// Breadth first
		array_t *dependencyTree = array_create();
		array_addObject(dependencyTree, library->dependencies);
		
		for(size_t i=0; i<array_count(dependencyTree); i++)
		{
			list_t *dependencies = array_objectAtIndex(dependencyTree, i);
			struct io_dependency_s *dependency = list_first(dependencies);
			while(dependency)
			{
				symbol = io_librarySymbolWithName(dependency->library, name, hash);
				if(symbol  && symbol->st_value != 0x0)
				{
					*outLib = dependency->library;
					array_destroy(dependencyTree);
					return symbol;
				}

				if(list_count(dependency->library->dependencies) > 0)
					array_addObject(dependencyTree, dependency->library->dependencies);
				
				dependency = dependency->next;
			}
		}

		array_destroy(dependencyTree);

		// Look it up in the library
		symbol = io_librarySymbolWithName(library, name, hash);
		if(symbol && symbol->st_value != 0x0)
		{
			*outLib = library;
			return symbol;
		}
	}

	return NULL;
}



int io_storeAtreeLookup(void *key1, void *key2)
{
	const char *name1 = (const char *)key1;
	const char *name2 = (const char *)key2;

	uint32_t index = 0;
	while(1)
	{
		char char1 = name1[index];
		char char2 = name2[index];

		if(char1 == char2)
		{
			if(char1 == '\0')
				return kCompareEqualTo;

			index ++;
			continue;
		}

		if(char1 > char2)
			return kCompareGreaterThan;

		if(char1 < char2)
			return kCompareLesserThan;
	}
}



io_library_t *io_storeLibraryWithName(const char *name)
{
	spinlock_lock(&__io_storeLock);
	io_library_t *library = (io_library_t *)atree_find(__io_storeLibraries, (void *)name);
	spinlock_unlock(&__io_storeLock);

	return library;
}

io_library_t *io_storeLibraryWithAddress(vm_address_t address)
{
	spinlock_lock(&__io_storeLock);
	iterator_t *iterator = atree_iterator(__io_storeLibraries);
	io_library_t *library;

	while((library = iterator_nextObject(iterator)))
	{
		vm_address_t vlimit = library->vmemory + (library->pages * VM_PAGE_SIZE);
		if(address >= library->vmemory && address <= vlimit)
		{
			iterator_destroy(iterator);
			spinlock_unlock(&__io_storeLock);

			return library;
		}
	}

	iterator_destroy(iterator);
	spinlock_unlock(&__io_storeLock);
	return NULL;
}

io_library_t *__io_storeLibraryWithAddress(vm_address_t address)
{
	if(spinlock_tryLock(&__io_storeLock))
	{
		iterator_t *iterator = atree_iterator(__io_storeLibraries);
		io_library_t *library;

		while((library = iterator_nextObject(iterator)))
		{
			vm_address_t vlimit = library->vmemory + (library->pages * VM_PAGE_SIZE);
			if(address >= library->vmemory && address <= vlimit)
			{
				iterator_destroy(iterator);
				spinlock_unlock(&__io_storeLock);

				return library;
			}
		}

		iterator_destroy(iterator);
		spinlock_unlock(&__io_storeLock);
	}
	
	return NULL;
}


bool io_storeAddLibrary(io_library_t *library)
{
	spinlock_lock(&__io_storeLock);
	if(!atree_find(__io_storeLibraries, (void *)library->name))
	{
		atree_insert(__io_storeLibraries, library, (void *)library->name);
		spinlock_unlock(&__io_storeLock);

		io_libraryResolveDependencies(library);

		bool rel1 = io_libraryRelocateNonPLT(library);
		bool rel2 = io_libraryRelocatePLT(library); 

		if(!rel1 || !rel2)
		{
			io_storeRemoveLibrary(library);
			return false;
		}

		return true;
	}

	spinlock_unlock(&__io_storeLock);
	return true;
}

void io_storeRemoveLibrary(io_library_t *library)
{
	spinlock_lock(&__io_storeLock);
	atree_remove(__io_storeLibraries, (void *)library->name);
	spinlock_unlock(&__io_storeLock);
}


typedef bool (*libio_init_t)();

void io_storeCallInitFunctions(io_library_t *library)
{
	for(size_t i=0; i<library->initArrayCount; i++)
	{
		uintptr_t ptr = library->initArray[i];
		if(ptr == 0x0 || ptr == UINT32_MAX)
			continue;

		io_module_init_t init = (io_module_init_t)ptr;
		init();
	}
}

bool io_initStubs();
bool io_moduleInit();

bool io_init(__unused void *data)
{
	if(!io_initStubs() || !io_moduleInit())
		return false;

	__io_storeLibraries = atree_create(io_storeAtreeLookup);
	if(!__io_storeLibraries)
		return false;

	if(sys_checkCommandline("--no-ioglue", NULL))
		return true;

	// Load the two essential kernel libraries
	__io_libkernel = io_libraryCreateWithFile("/lib/libkernel.so");
	__io_libio     = io_libraryCreateWithFile("/lib/libio.so");

	if(__io_libkernel && __io_libio)
	{
		// Relocate and link into the kernel
		bool result1 = io_storeAddLibrary(__io_libkernel);
		bool result2 = io_storeAddLibrary(__io_libio);
		
		if(!result1 || !result2)
		{
			if(!result1)
				dbg("Couldn't add libkernel.so to the iostore!");

			if(!result2)
				dbg("Couldn't add libio.so to the iostore!");

			return false;
		}
		
		io_storeCallInitFunctions(__io_libkernel);
		io_storeCallInitFunctions(__io_libio);

		libio_init_t libio_init = (libio_init_t)io_libraryFindSymbol(__io_libio, "libio_init");
		if(!libio_init)
		{
			dbg("libio_init() not found in libio!");
			return false;
		}

		dbg("libkernel.so at \16\27%p\16\31, libio.so at \16\27%p\16\31", __io_libkernel->relocBase, __io_libio->relocBase);
		return libio_init();
	}

	return false;
}
