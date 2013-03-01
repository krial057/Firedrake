//
//  syscall.c
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

#include <interrupts/interrupts.h>
#include <scheduler/scheduler.h>
#include <memory/memory.h>
#include <system/syslog.h>
#include <libc/assert.h>
#include <system/video.h>
#include "syscall.h"

static syscall_callback_t _sc_syscalls[_SYS_MAXCALLS];

// Mark: Implementation
uint32_t _sc_print(uint32_t *UNUSED(esp), uint32_t *uesp, int *errno)
{
	vm_address_t virtual;
	char *string = sc_mapProcessMemory(*(void **)(uesp), &virtual, 2, errno);
	if(!string)
		return -1;

	info("%s", string);

	vm_free(vm_getKernelDirectory(), virtual, 2);
	return 0;
}


/**
 * Syscall main entry point
 * Responsible for mapping the user stack into the kernel page directory and calling the actual syscall handler
 **/
uint32_t _sc_execute(uint32_t esp)
{
	cpu_state_t *state = (cpu_state_t *)esp;
	syscall_callback_t callback = _sc_syscalls[state->eax];

	if(callback == NULL)
	{
		dbg("Syscall %i not implemented but invoked!\n", state->eax);
		return esp;
	}

	// Map the userstack
	thread_t *thread = thread_getCurrentThread();
	uint32_t *ustack = (uint32_t *)vm_alloc(vm_getKernelDirectory(), (uintptr_t)thread->userStack, 1, VM_FLAGS_KERNEL);
	uint32_t offset = ((uint8_t *)state->esp) - thread->userStackVirt;

	uint32_t *uesp = (uint32_t *)(((uint8_t *)ustack) + offset); // The mapped user stack
	uesp ++; // Return address of the syscall callee
	uesp ++; // Syscall number, same as in eax

	// Call the syscall handler
	int errno = 0;
	uint32_t result = callback(&esp, uesp, &errno);

	state->eax = result;
	state->ecx = (errno != 0) ? errno : state->ecx;

	thread->errno = errno;
	
	// Unmap the userstack
	vm_free(vm_getKernelDirectory(), (uintptr_t)ustack, 1);
	return esp;
}

void sc_setSyscallHandler(uint32_t syscall, syscall_callback_t callback)
{
	assert(syscall >= 0 && syscall < _SYS_MAXCALLS);
	_sc_syscalls[syscall] = callback;
}

void _sc_processInit();
void _sc_threadInit();
void _sc_mmapInit();

bool sc_init(void *UNUSED(ingored))
{
	ir_setInterruptHandler(_sc_execute, 0x80);

	sc_setSyscallHandler(SYS_PRINT, _sc_print);
	sc_setSyscallHandler(SYS_PRINTCOLOR, _sc_print);

	_sc_processInit();
	_sc_threadInit();
	_sc_mmapInit();

	return true;
}
