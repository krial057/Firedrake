//
//  loader.h
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

#ifndef _LOADER_H_
#define _LOADER_H_

#include <prefix.h>
#include <memory/memory.h>

typedef struct ld_exectuable_s
{
	vm_page_directory_t pdirectory;
	struct ld_exectuable_s *source;

	vm_address_t entry; // The main entry point into the executable

	// Location and size of the image
	uintptr_t    	pimage;
	vm_address_t 	vimage;
	size_t 			imagePages;

	uint32_t useCount;
} ld_exectuable_t;

ld_exectuable_t *ld_exectuableCreate(vm_page_directory_t pdirectory,  uint8_t *begin, size_t size);
ld_exectuable_t *ld_executableCreateWithFile(vm_page_directory_t pdirectory, const char *file);
ld_exectuable_t *ld_exectuableCopy(vm_page_directory_t pdirectory, ld_exectuable_t *source);

void ld_executableRelease(ld_exectuable_t *executable);

#endif /* _LOADER_H_ */
