//
//  IPCPort.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
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

#include <os/scheduler/task.h>
#include <objects/IONumber.h>
#include "IPCPort.h"
#include "IPCMessage.h"

namespace OS
{
	namespace IPC
	{
		IODefineMeta(Port, IO::Object)

		Port *Port::Init(System *system, uint16_t name, Rights rights)
		{
			if(!IO::Object::Init())
				return nullptr;

			_rights = rights;
			_name = name;
			_system = system;
			_queue = (rights & Rights::Receive) ? IO::Array::Alloc()->Init() : nullptr;

			Task *task = _system->GetTask();
			_portName = IPCCreatePortName(task->GetPid(), _system->GetName(), _name);

			return this;
		}
		Port *Port::InitAsPortRight(Port *port, uint16_t name)
		{
			if(!IO::Object::Init())
				return nullptr;

			_name = name;
			_rights = port->_rights;
			_system = port->_system;
			_queue = nullptr;

			Task *task = _system->GetTask();
			_portName = IPCCreatePortRight(task->GetPid(), _system->GetName(), _name);

			return this;
		}

		void Port::Dealloc()
		{
			IO::SafeRelease(_queue);
			IO::Object::Dealloc();
		}
		

		void Port::PushMessage(Message *msg)
		{
			IOAssert(_rights & Rights::Receive, "Port must be a receive port");
			_queue->AddObject(msg);
		}
		Message *Port::PeekMessage()
		{
			IOAssert(_rights & Rights::Receive, "Port must be a receive port");

			if(_queue->GetCount() == 0)
				return nullptr;

			return _queue->GetFirstObject<Message>();
		}
		void Port::PopMessage()
		{
			IOAssert(_rights & Rights::Receive, "Port must be a receive port");
			_queue->RemoveObjectAtIndex(0);
		}
	}
}
