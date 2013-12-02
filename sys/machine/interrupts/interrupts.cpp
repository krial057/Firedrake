//
//  interrupts.cpp
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

#include <machine/port.h>
#include <machine/cpu.h>
#include <kern/kprintf.h>
#include <kern/panic.h>
#include "interrupts.h"
#include "trampoline.h"
#include "apic.h"

extern "C" uint32_t ir_handle_interrupt(uint32_t esp);
static ir::interrupt_handler_t _ir_interrupt_handler[IDT_ENTRIES];

void ir_panic_segfault()
{
	uint32_t address;
	__asm__ volatile("movl %%cr2, %0" : "=r" (address));

	panic("Segfault at address %p", (void *)address);
}

uint32_t ir_handle_interrupt(uint32_t esp)
{
	cpu_state_t *state = reinterpret_cast<cpu_state_t *>(esp);
	cpu_t *cpu = cpu_get_current_cpu();

	bool needsEOI = true;

	cpu_state_t *prev = cpu->last_state;
	cpu->last_state = state;

	switch(state->interrupt)
	{
		case 0x27:
		case 0x2f:
		case 0x32:
			needsEOI = false; // Spurious interrupts
			break;

		case 0x39:
			// Shutdown CPU
			while(1)
			{
				cli();
				cpu_halt();
			}

			break;

		case 0x8:
			panic("Double fault!\n");
			break;

		case 0xe:
			ir_panic_segfault();
			break;

		default:
		{
			ir::interrupt_handler_t handler = _ir_interrupt_handler[state->interrupt];
			if(handler)
				handler(state->interrupt, cpu);

			break;
		}
	}

	cpu->last_state = prev;

	if(needsEOI)
		ir::apic_write(APIC_REGISTER_EOI, 0);

	return esp;
}

namespace ir
{
	// --------------------
	// MARK: -
	// MARK: IDT
	// --------------------

	void idt_set_entry(uint64_t *idt, uint32_t index, uint32_t handler, uint32_t selector, uint32_t flags)
	{
		idt[index] = handler & UINT64_C(0xffff);
		idt[index] |= (selector & UINT64_C(0xffff)) << 16;
		idt[index] |= (flags & UINT64_C(0xff)) << 40;
		idt[index] |= ((handler>> 16) & UINT64_C(0xffff)) << 48;
	}

#define idt_set_interrupt_entry(num, flags) \
	do{ idt_set_entry(idt, num, (reinterpret_cast<uint32_t>(idt_interrupt_ ## num)) + offset, 0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_PRESENT | flags); } while(0)

#define idt_set_interrupt_set(num, flags) \
		idt_set_interrupt_entry(num ## 0, flags); \
		idt_set_interrupt_entry(num ## 1, flags); \
		idt_set_interrupt_entry(num ## 2, flags); \
		idt_set_interrupt_entry(num ## 3, flags); \
		idt_set_interrupt_entry(num ## 4, flags); \
		idt_set_interrupt_entry(num ## 5, flags); \
		idt_set_interrupt_entry(num ## 6, flags); \
		idt_set_interrupt_entry(num ## 7, flags); \
		idt_set_interrupt_entry(num ## 8, flags); \
		idt_set_interrupt_entry(num ## 9, flags); \
		idt_set_interrupt_entry(num ## a, flags); \
		idt_set_interrupt_entry(num ## b, flags); \
		idt_set_interrupt_entry(num ## c, flags); \
		idt_set_interrupt_entry(num ## d, flags); \
		idt_set_interrupt_entry(num ## e, flags); \
		idt_set_interrupt_entry(num ## f, flags)

	void idt_init(uint64_t *idt, uint32_t offset)
	{
		// Excpetion-Handler
		idt_set_entry(idt, 0x0, (reinterpret_cast<uint32_t>(idt_exception_divbyzero)) + offset,             0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x1, (reinterpret_cast<uint32_t>(idt_exception_debug)) + offset,                 0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x3, (reinterpret_cast<uint32_t>(idt_exception_breakpoint)) + offset,            0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x4, (reinterpret_cast<uint32_t>(idt_exception_overflow)) + offset,              0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x5, (reinterpret_cast<uint32_t>(idt_exception_boundrange)) + offset,            0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x6, (reinterpret_cast<uint32_t>(idt_exception_opcode)) + offset,                0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x7, (reinterpret_cast<uint32_t>(idt_exception_devicenotavailable)) + offset,    0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x8, (reinterpret_cast<uint32_t>(idt_exception_doublefault)) + offset,           0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x9, (reinterpret_cast<uint32_t>(idt_exception_segmentoverrun)) + offset,        0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0xa, (reinterpret_cast<uint32_t>(idt_exception_invalidtss)) + offset,            0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0xb, (reinterpret_cast<uint32_t>(idt_exception_segmentnotpresent)) + offset,     0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0xc, (reinterpret_cast<uint32_t>(idt_exception_stackfault)) + offset,            0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0xd, (reinterpret_cast<uint32_t>(idt_exception_protectionfault)) + offset,       0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0xe, (reinterpret_cast<uint32_t>(idt_exception_pagefault)) + offset,             0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x10, (reinterpret_cast<uint32_t>(idt_exception_fpuerror)) + offset,             0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x11, (reinterpret_cast<uint32_t>(idt_exception_alignment)) + offset,            0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x12, (reinterpret_cast<uint32_t>(idt_exception_machinecheck)) + offset,         0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		idt_set_entry(idt, 0x13, (reinterpret_cast<uint32_t>(idt_exception_simd)) + offset,                 0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	
		// Interrupts
		idt_set_interrupt_entry(0x02, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x20, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x21, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x22, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x23, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x24, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x25, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x26, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x27, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x28, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x29, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x2a, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x2b, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x2c, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x2d, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x2e, IDT_FLAG_RING0);
		idt_set_interrupt_entry(0x2f, IDT_FLAG_RING0);

		idt_set_interrupt_set(0x3, IDT_FLAG_RING0);
		idt_set_interrupt_set(0x4, IDT_FLAG_RING0);
		idt_set_interrupt_set(0x5, IDT_FLAG_RING0);
		idt_set_interrupt_set(0x6, IDT_FLAG_RING0);
		idt_set_interrupt_set(0x7, IDT_FLAG_RING0);
		idt_set_interrupt_set(0x8, IDT_FLAG_RING3);
		idt_set_interrupt_set(0x9, IDT_FLAG_RING0);
		idt_set_interrupt_set(0xa, IDT_FLAG_RING0);
		idt_set_interrupt_set(0xb, IDT_FLAG_RING0);
		idt_set_interrupt_set(0xc, IDT_FLAG_RING0);
		idt_set_interrupt_set(0xd, IDT_FLAG_RING0);
		idt_set_interrupt_set(0xe, IDT_FLAG_RING0);
		idt_set_interrupt_set(0xf, IDT_FLAG_RING0);

		// Reload the IDT
		struct 
		{
			uint16_t limit;
			void *pointer;
		} __attribute__((packed)) idtp;

		idtp.limit = (IDT_ENTRIES * sizeof(uint64_t)) - 1,
		idtp.pointer = idt;

		__asm__ volatile("lidt %0" : : "m" (idtp));
	}

	// --------------------
	// MARK: -
	// MARK: Interrupt handler
	// --------------------

	kern_return_t set_interrupt_handler(uint8_t vector, interrupt_handler_t handler)
	{
		_ir_interrupt_handler[vector] = handler;
		return KERN_SUCCESS;
	}

	// --------------------
	// MARK: -
	// MARK: Initialization
	// --------------------

	kern_return_t init()
	{
		kern_return_t result;

		cli();

		kprintf("apic");
		if((result = apic_init()) != KERN_SUCCESS)
			return result;

		kprintf(", trampoline");
		if((result = trampoline_init()) != KERN_SUCCESS)
			return result;

		sti();
		return KERN_SUCCESS;
	}

	kern_return_t init_application_cpu()
	{
		kern_return_t result;

		cli();

		if((result = apic_init_cpu()) != KERN_SUCCESS)
			return result;

		if((result = trampoline_init_cpu()) != KERN_SUCCESS)
			return result;

		sti();
		return KERN_SUCCESS;
	}
}
