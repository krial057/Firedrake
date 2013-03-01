//
//  syscall.h
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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>
#include <errno.h>
#include <system/cpu.h>

#define _SYS_MAXCALLS 128 // Increase if needed

#define SYS_PRINT         0
#define SYS_PRINTCOLOR    1
#define SYS_EXIT          2
#define SYS_YIELD         3
#define SYS_SLEEP         18
#define SYS_THREADATTACH  4
#define SYS_THREADEXIT    5
#define SYS_THREADJOIN    6
#define SYS_ERRNO         13
#define SYS_TLS_ALLOCATE  14
#define SYS_TLS_FREE      15
#define SYS_TLS_SET       16
#define SYS_TLS_GET       17
#define SYS_PROCESSCREATE 7
#define SYS_PROCESSKILL   8
#define SYS_MMAP          9
#define SYS_MUNMAP        10
#define SYS_MPROTECT      11
#define SYS_FORK          12

static inline void *sc_mapProcessMemory(void *memory, vm_address_t *mappedBase, size_t pages, int *errno)
{
	process_t *process = process_getCurrentProcess();

	vm_address_t virtual = round4kDown((vm_address_t)memory);
	vm_address_t offset  = ((vm_address_t)memory) - virtual;
	uintptr_t physical;

	if(virtual == 0x0)
	{
		*errno = EINVAL;
		return NULL;
	}

	physical = vm_resolveVirtualAddress(process->pdirectory, virtual);
	virtual  = vm_alloc(vm_getKernelDirectory(), physical, pages, VM_FLAGS_KERNEL);

	if(!virtual)
	{
		*errno = ENOMEM;
		return NULL;
	}

	*mappedBase = virtual;
	return (void *)(virtual + offset);
}

typedef uint32_t (*syscall_callback_t)(uint32_t *esp, uint32_t *uesp, int *errno);

void sc_setSyscallHandler(uint32_t syscall, syscall_callback_t callback);
bool sc_init(void *ingored);

#endif
