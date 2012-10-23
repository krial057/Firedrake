//
//  boot.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2012 by Sidney Just
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

#include "multiboot.h"

#include <version.h>
#include <types.h>
#include <config.h>
#include <kerneld/syslogd.h>
#include <system/syslog.h>
#include <system/panic.h>
#include <system/video.h>
#include <system/kernel.h>

#include <interrupts/interrupts.h>
#include <interrupts/trampoline.h>
#include <syscall/syscall.h>
#include <memory/memory.h>
#include <kerneld/watchdogd.h>
#include <scheduler/scheduler.h>
#include <ioglue/iostore.h>
#include <system/time.h>

struct multiboot_s *bootinfo = NULL;
typedef bool (*sys_function_t)(void *);

// Simple macro to initialize a system module. Its only purpose is to print a common message and abort the boot if the module is marked essential.
void sys_init(char *name, sys_function_t function, void *data, bool essential)
{
	syslog(LOG_DEBUG, "Initializing %s... {", name);
	if(function(data))
	{
		syslog(LOG_DEBUG, "} ok\n");
	}
	else
	{
		syslog(LOG_DEBUG, "} failed\n");

		if(essential)
			panic("Failed to initialize essential module '%s'!", name);
	}
}

extern void _vd_init(); // Declared in system/video.c
extern void kerneld_main(); // Declared in kerneld/kerneld.c
extern void time_waitForBootTime(); // Declared in system/time.c

void sys_boot(struct multiboot_s *info) __attribute__ ((noreturn));
void sys_boot(struct multiboot_s *info)
{	
	bootinfo = info;
	_vd_init();
	
#ifndef CONF_RELEASE
	syslogd_setLogLevel(LOG_DEBUG);
#else
	syslogd_setLogLevel(LOG_INFO);
#endif
	
	syslog(LOG_INFO, "Firedrake v%i.%i.%i:%i%s (%s)\n", VersionMajor, VersionMinor, VersionPatch, VersionCurrent, versionAppendix, versionBeast);
	syslog(LOG_INFO, "Kernel compiled on %s %s\n", __DATE__, __TIME__);
	syslog(LOG_INFO, "Here be dragons!\n\n");

	// Color the dragon!
	vd_setColor(0, 0, true, vd_color_red, 9);

	// Load the modules
	sys_init("physical memory", pm_init, (void *)info, true);
	sys_init("virtual memory", vm_init, (void *)info, true); // After this point, never ever use unmapped memory again! Note that this also maps the multiboot info and the modules, but not the memory information!
	sys_init("heap allocator", heap_init, NULL, true);
	sys_init("interrupts", ir_init, NULL, true); // Requires memory
	sys_init("watchdog", watchdogd_init, NULL, true); // Requires memory
	sys_init("time keeping", time_init, NULL, true); // Requires that interrupts are disabled, including NMI. So must be done before the scheduler kicks in and enables them
	sys_init("scheduler", sd_init, NULL, true); // Requires interrupts!
	sys_init("syscalls", sc_init, NULL, true); // Requires interrupts!
	sys_init("ioglue", io_init, NULL, true);

	dbg("\n");
	time_waitForBootTime(); // Wait until we have time ready

#ifndef CONF_RELEASE
	info("--------------------------------------------------------------------------------\n\n");
#else
	vd_clear();
#endif

	kerneld_main(); // Jump over to the kernel daemon which will do the rest of the work now
	panic("kerneld bugged"); // In the case that we leave kerneld, something went really wrong. PANIC!
}