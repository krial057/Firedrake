//
//  sys/unistd.h
//  libc
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

#ifndef _SYS_UNISTD_H_
#define _SYS_UNISTD_H_

#include "types.h"

#define MAXNAME 256

#define DTREG 0
#define DTDIR 1
#define DTLNK 2

struct stat
{
	int type;
	char name[MAXNAME];

	ino_t  id;
	size_t size;

	timestamp_t atime;
	timestamp_t mtime;
	timestamp_t ctime;
};

int open(const char *path, int flags);
void close(int fd);

size_t read(int fd, void *buffer, size_t count);
size_t write(int fd, const void *buffer, size_t count);
off_t lseek(int fd, off_t offset, int whence);

int mkdir(const char *path);
int remove(const char *path);
int move(const char *source, const char *target);
int stat(const char *path, struct stat *buf);

pid_t getpid();
pid_t getppid();

pid_t fork();

void waitpid(pid_t pid);

#endif
