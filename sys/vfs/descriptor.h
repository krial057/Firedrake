//
//  descriptor.h
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

#ifndef _VFS_DESCRIPTOR_H_
#define _VFS_DESCRIPTOR_H_

#include <prefix.h>
#include <kern/spinlock.h>
#include <kern/kern_return.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libcpp/bitfield.h>
#include <libcpp/string.h>
#include <libcpp/vector.h>

namespace VFS
{
	class Instance;
	class Descriptor
	{
	public:
		struct Flags : cpp::Bitfield<uint32_t>
		{
			Flags()
			{}
			Flags(int value) :
				Bitfield(value)
			{}

			enum
			{
				Persistent = (1 << 0)
			};
		};

		virtual ~Descriptor();

		const char *GetName() const { return _name.c_str(); }
		Flags GetFlags() const { return _flags; }

		virtual kern_return_t CreateInstance(Instance *&instance) = 0;
		virtual void DestroyInstance(Instance *instance) = 0;

		// Must be called at some point
		kern_return_t Register();
		kern_return_t Unregister();

	protected:
		Descriptor(const char *name, Flags flags);

		// Must be called while locked
		void AddInstance(Instance *instance);
		bool RemoveInstance(Instance *instance);

		void Lock();
		void Unlock();

	private:
		std::fixed_string<32> _name;
		spinlock_t _lock;
		Flags _flags;

		bool _registered;

		std::vector<Instance *> _instances;
	};
}

#endif /* _VFS_DESCRIPTOR_H_ */