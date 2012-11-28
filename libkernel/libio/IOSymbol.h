//
//  IOSymbol.h
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

#ifndef _IOSYMBOL_H_
#define _IOSYMBOL_H_

#include "IOTypes.h"

class IOObject;
class IOString;

/**
 * @brief Provides introspection and runtime support
 *
 * libio's runtime creates an IOSymbol instance for every class that derives from
 * IOObject and registers it with the internal symbol database. Once registered, symbols
 * can be looked up at runtime using the class name. You can also check the symbol of
 * an object by calling IOObject::symbol(), the returned symbol can be used for inheritance check.
 * An IOSymbol can also be used to create new instances of the encapsulated class by using the 
 * alloc() method.
 **/
class IOSymbol
{
public:
	/**
	 * Returns the symbol of the class with the given name or 0
	 **/
	static IOSymbol *withName(const char *name);
	/**
	 * Returns the symbol of the class with the given name or 0
	 **/
	static IOSymbol *withName(IOString *name);

	/**
	 * Returns true if the encapsulated class inherits from the class encapuslated by the other symbol
	 **/
	bool inheritsFrom(IOSymbol *other);

	/**
	 * Returns the name of the encapsulated class
	 **/
	const IOString *name() const;
	/**
	 * Returns the size of the encapsulated class in bytes
	 **/
	size_t size() const;

	/**
	 * Creates a new instance of the encapsulated class and returns it
	 **/
	IOObject *alloc();
	/**
	 * Returns the symbol of the super class, or 0 if there is no super class
	 **/
	IOSymbol *super();

	/**
	 * @internal
	 * Callback used to create a new instance of a class, usually points to the static alloc() method of the class.
	 **/
	typedef IOObject *(*AllocCallback)();

	/**
	 * @internal
	 * Constructor
	 * @param name The name of the class
	 * @param super The name of the superclass
	 * @param size The size of the class
	 * @param callback Callback to be used when requesting a new instance of the class
	 **/
	IOSymbol(const char *name, const char *super, size_t size, AllocCallback callback);

private:
	IOString *_name;
	IOSymbol *_super;

	AllocCallback _allocCallback;
	const char *_superName;
	size_t _size;
};

#endif
