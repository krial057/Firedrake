//
//  string.c
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

#include "string.h"

void *memset(void *dst, int c, size_t size)
{
	if(size)
	{
		unsigned char *d = (unsigned char *)dst;

		do {
			*d++ = (unsigned char)c;
		} while(--size != 0);
	}

	return dst;
}

void *memcpy(void *dst, const void *src, size_t size)
{
	if(size)
	{
		unsigned char *d = (unsigned char *)dst;
		unsigned char *s = (unsigned char *)src;

		do {
			*d++ = *s++;
		} while(--size != 0);
	}

	return dst;
}

char *strcpy(char *dst, const char *src)
{
	char *d = dst;

	while(1)
	{
		*dst = *src;

		if(*src == '\0')
			break;

		dst ++;
		src ++;
	}

	return d;
}

size_t strlcpy(char *dst, const char *src, size_t size)
{
	const char *s = src;
	char *d = dst;
	size_t n = size;

	if(n) 
	{
		// Copy as many characters as possible from src to dst.
		do {	
			if((*d++ = *s++) == '\0')
				break;
				
		} while(--n != 0);
	}


	if(n == 0) 
	{
		if(size)
			*d = '\0'; // Add the missing NUL termination

		while(*s++ != '\0')
		{} // Traverse to the end of s
	}

	return (s - src) - 1; // Return the count of copied bytes, not including the NULL termination
}

size_t strlen(const char *string)
{
	const char *s = string;
	for(; *s != '\0'; s++)
	{}

	return s - string;
}

int strcmp(const char *str1, const char *str2)
{
	if(!str1)
		return -1;

	if(!str2)
		return 1;

	while(1)
	{
		if(*str1 == *str2)
		{
			if(*str1 == '\0')
				break;

			str1 ++;
			str2 ++;

			continue;
		}

		return *str2 - *str1;
	}

	return 0;
}

int strncmp(const char *str1, const char *str2, size_t size)
{
	if(!str1)
		return -1;

	if(!str2)
		return 1;

	for(size_t i=0; i<size; i++)
	{
		if(*str1 == *str2)
		{
			if(*str1 == '\0')
				break;

			str1 ++;
			str2 ++;

			continue;
		}

		return *str2 - *str1;
	}

	return 0;
}

char *strstr(char *str1, const char *str2)
{
	size_t index = 0;
	size_t length = strlen(str2);
	char *start = NULL;
	
	for(; *str1 != '\0'; str1++)
	{
		if(*str1 == str2[index])
		{
			if(index == 0)
				start = str1;
			
			index ++;
			if(index == length)
				return start;
		}
		else
		{
			index = 0;
		}
	}
	
	return NULL;
}

char *strpbrk(char *str1, const char *str2)
{
	size_t length = strlen(str2);
	
	for(; *str1 != '\0'; str1++)
	{
		for(size_t i=0; i<length; i++)
		{
			if(*str1 == str2[i])
				return str1;
		}
	}
	
	return NULL;
}
