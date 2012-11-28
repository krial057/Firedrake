//
//  IOObject.h
//  libio
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

#ifndef _IOOBJECT_H_
#define _IOOBJECT_H_

#include <libkernel/libkernel.h>
#include "IOTypes.h"
#include "IOSymbol.h"
#include "IORuntime.h"

class IOString;
class IODatabase;

/**
 * @brief Root class for libio objects
 *
 * The IOObject class implements the most basic functionality shared by all objects
 * within the kernel. It provides C++ untypical runtime methods like introspection and
 * dynamic allocation (with help of the IOSymbol class). It also uses reference counting
 * for memory managment via the retain(), release() and autorelease() methods.
 *
 * ### Instancing
 * IOObject subclasses are instanced using the static alloc() method, which returns an uninitialized
 * object with a retain count of 1. The next step is usually to call one of the subclasses init() methods
 * which is responsible to initialize the object. Most classes also provide static factory methods
 * for easier object creation in one step. These factory methods usually start with `class::with`, eg `IOArray::withCapacity()`
 * and always return an autoreleased instance.
 *
 * Another way of object creation is the use of the IOSymbol class, which provides means of looking up
 * classes at runtime and instancing. Example:
 *
 * 	IOSymbol *symbol = IOSymbol::symbolWithName("IOArray"); // Grab the symbol for the IOArray class
 * 	IOArray *array = (IOArray *)symbol->alloc(); // Create a new instance
 * 	array->init(); // Initialize the array
 *
 * ### Subclassing
 * Most libio classes expect to work with IOObject subclasses (that's especially true for the container classes),
 * so while you are not required to subclass from IOObject, it's highly recommended that you do it if you intend to
 * pass your instances around. Subclassing from IOObject is straightforward, however, you have to add two things to your
 * code in order to make the object work properly with the dynamic runtime. First of all, you have to declare your class
 * using the IODeclareClass() macro which must be placed inside your class. Then you have to register the class using the
 * IORegisterClass() macro which should be placed inside the .cpp file. Example:
 *
 * 	// mySublcass.h
 * 	class mySubclass : public IOObject
 * 	{
 * 	public:
 * 		// Class content here
 * 	
 * 	private:
 * 		IODeclareClass(mySubclass)
 * 	};
 * 
 * 
 * 	// mySubclass.cpp
 * 	#include "mySubclass.h"
 * 	
 * 	IORegisterClass(mySubclass, IOObject);
 *
 * Note also that classes that derive from IOObject must only inherit from one class, multiple inheritance is NOT
 * supported and will break!
 *
 * ### Memory managment
 * IOObject uses a reference counting model for memory management. This means that every object has an internal 
 * counter of owners, called the retain count. When instancing an object, it starts with a retain count of
 * 1, and the creator takes ownership of the object. Each owner must relinquish it's ownership of the object
 * once it doesn't need it anymore using the release() method. On the other hand, you can specify interest in an
 * object (and take it's ownership) by calling its retain() method. retain() increases the retain count of the object
 * by 1, while release() decreases it by one, so every retain() must be balanced with a release(). Once the retain count
 * of the object hits zero, it's free() method is called and it's memory released. <br />
 * Methods that don't contain `copy`, `new` or `alloc` in their names must return an autoreleased object instead of
 * delegating the ownership to the caller. On the other hand, methods that do contain `copy`, `new` or `alloc` must delegate
 * the ownership to the caller.
 *
 * ### Introspection
 * IOObject provides introspection using the IOSymbol class. Each IOObject subclass has it's own IOSymbol
 * which can be used to check wether another object is derived from the same class. Like already mentioned,
 * it's also possible to create objects dynamically using the IOSymbol class.  
 *
 **/
class IOObject
{
friend class IOSymbol;
friend class IODatabase;
public:
	/**
	 * Decerements the retain count by one.
	 * @remark If the retain count hits zero, the object will be freed and it's memory released
	 * @remark It is safe to call this method on a NULL object
	 * @sa autorelease
	 * @sa retain
	 * @sa free
	 **/
	void release();
	/**
	 * Increments the retain count by one.
	 * @remark Every retain call must be balanced by a release call once the object is no longer needed.
	 * @return The pointer to the object. Can be used to chain method calls like `foo = bar->retain();`
	 * @remark It is safe to call this method on a NULL object
	 * @sa release
	 * @sa autorelease
	 **/
	virtual IOObject *retain();
	/**
	 * @brief Marks the object to be released in the future
	 *
	 * The autorelease method adds the object to the IOAutoreleasePool of the calling thread.
	 * It will receive a release method for everytime it was added once the autorelease pool gets drained.
	 * @return The pointer to the object. Can be used to chain method calls like `return bar->autorelease();`
	 * @remark This method is useful if you want to return an object without retaining ownership of it
	 * @remark It is safe to call this method on a NULL object
	 * @sa release
	 * @sa retain
	 * @sa free
	 **/
	virtual IOObject *autorelease();

	/**
	 * Returns a new object.
	 * @remark Use this instead of the new operator
	 * @remark Note that the caller receives the ownership of the object created
	 * @remark Note that the object isn't initialized, you still need to call the appropriate init method.
	 **/
	static IOObject *alloc();

	/**
	 * Returns the global IOSymbol of the object
	 * @sa IOSymbol
	 **/
	IOSymbol *symbol() const;

	/**
	 * Returns the hash of the object. Subclasses should override this method
	 * @remark The hash of an object must not change unless the object is mutated, 
	 * ie subsequent calls to this method must return the same value
	 * @remark Two objects must not be identical if they return a different hash. 
	 * However, two objects might be identical if they return the same hash.
	 * @sa isEqual
	 **/
	virtual hash_t hash() const;
	/**
	 * Returns true if two objects are equal. Subclasses should override this method.
	 * @param other The object that should be compared to this object
	 * @remark If two objects compare equal, they must also have the same hash() value
	 * @sa hash
	 **/
	virtual bool isEqual(const IOObject *other) const;

	/**
	 * Returns true if the object is derived from the same class as `other`
	 * @param other The object to check against
	 **/
	bool isKindOf(IOObject *other) const;
	/**
	 * Returns true if the objects class is derived of the class represented by symbol
	 **/
	bool isSubclassOf(IOSymbol *symbol) const;
	/**
	 * Same as calling isSubclassOf(IOSymbol::symbolWithName(name));
	 **/
	bool isSubclassOf(const char *name) const;

	/**
	 * Returns a debug description. If not overwritten, the returned string contains
	 * the name of the class and the pointer, eg `<IOArray:0x1337>`.
	 * @remark The returned string is used by the IOLog() and IOString format functions, 
	 * using %@ as format specifier and passing an object will query the description method and print the result
	 **/
	virtual IOString *description() const;

protected:
	/**
	 * Designated initializer.
	 * @remark When subclassing IOObject, you should override this initializer, and/or provide your own intializer(s)
	 * if required.
	 * @remark Subclasses are free to change the designated initializer (or drop it completely)
	 **/
	virtual IOObject *init();
	/**
	 * Called when an object is about to be deleted. Subclasses should override this method and
	 * release all memory that the class allocated.
	 * @remark If you override this method, you have to call free() on the super class.
	 **/
	virtual void free();

	/**
	 * @internal
	 * Used to initialize new instances with a retain count of 1 and the correct symbol.
	 * Automatically called by the alloc() method.
	 **/
	void prepareWithSymbol(IOSymbol *symbol);

private:
	IOSymbol *_symbol;
	uint32_t _retainCount;
	kern_spinlock_t _lock;
};

#define __IOTokenExpand(tok) #tok
#define __IOToken(tok) __IOTokenExpand(tok)

#define IODeclareClass(className) \
	public: \
		virtual className *retain(); \
		virtual className *autorelease(); \
		static className *alloc(); \
	private: \

#define IORegisterClass(className, super) \
	static IOSymbol *__##className##__symbol = 0; \
	className *className::alloc() \
	{ \
		className *instance = new className; \
		instance->prepareWithSymbol(__##className##__symbol); \
		return instance; \
	} \
	className *className::retain() \
	{ \
		return (className *)IOObject::retain(); \
	} \
	className *className::autorelease() \
	{ \
		return (className *)IOObject::autorelease(); \
	} \
	void __##className##__load() __attribute((constructor)); \
	void __##className##__load() \
	{ \
		__##className##__symbol = new IOSymbol(#className, __IOToken(super), sizeof(className), (IOSymbol::AllocCallback)&className::alloc); \
	}

#endif /* _IOOBJECT_H_ */
