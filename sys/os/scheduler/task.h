//
//  task.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#ifndef _TASK_H_
#define _TASK_H_

#include <prefix.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libc/sys/types.h>
#include <libc/sys/spinlock.h>
#include <libcpp/atomic.h>
#include <libcpp/queue.h>
#include <machine/memory/memory.h>
#include <kern/kern_return.h>
#include <os/loader/loader.h>
#include <objects/IOObject.h>
#include <objects/IOArray.h>

#include "thread.h"

namespace VFS
{
	class Context;
	class File;
}

namespace OS
{
	class Task : public IO::Object
	{
	public:
		friend class Thread;
		friend class VFS::Context;

		enum class State
		{
			Waiting,
			Running,
			Blocked,
			Died
		};

		KernReturn<Task *> Init();
		KernReturn<Task *> InitWithFile(const char *path);

		KernReturn<Thread *> AttachThread( Thread::Entry entry, size_t stack);

		void Lock();
		void Unlock();

		pid_t GetPid() const { return _pid; }
		Sys::VM::Directory *GetDirectory() const { return _directory; }

		// VFS
		VFS::Context *GetVFSContext() const { return _context; }

		VFS::File *GetFileForDescriptor(int fd);
		void SetFileForDescriptor(VFS::File *file, int fd);

		int AllocateFileDescriptor();
		void FreeFileDescriptor(int fd);

	protected:
		void Dealloc() override;

	private:
		void RemoveThread(Thread *thread);

		Sys::VM::Directory *_directory;
		Executable *_executable;

		std::atomic<int32_t> _tidCounter;
		IO::Array *_threads;
		Thread *_mainThread;

		spinlock_t _lock;
		pid_t _pid;
		State _state;

		bool _ring3;

		VFS::Context *_context;
		VFS::File *_files[CONFIG_MAX_FILES];

		IODeclareMeta(Task)
	};
}

#endif /* _TASK_H_ */