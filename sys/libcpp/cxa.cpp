//
//  cxa.cpp
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

#include <prefix.h>

void *__dso_handle;

extern "C"
{
	void __cxa_pure_virtual()
	{
		// TODO: Panic
		while(1)
		{
			__asm__ volatile("cli; hlt;");
		}
	}


	int __cxa_atexit(__unused void (*f)(void *), __unused void *p, __unused void *d)
	{
		// The kernel will be alive for the whole livetime of the machine
		// so there is not much sense in actually destructing anything in __cxa_finalize
		// so no need to track anything here.
		return 0;
	}

	void __cxa_finalize(__unused void *d)
	{}
};

typedef void (*cxa_constructor_t)();

extern "C" cxa_constructor_t ctors_begin;
extern "C" cxa_constructor_t ctors_end;

void cxa_init()
{
	cxa_constructor_t *iterator = &ctors_begin;
	cxa_constructor_t *end = &ctors_end;

	while(iterator != end)
	{
		(*iterator)();
		iterator ++;
	}
}