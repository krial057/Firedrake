//
//  setjmp.S
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

.section .text

.global setjmp
setjmp:
	movl 4(%esp), 	%ecx
	movl %ebx,		0(%ecx)
	movl %esi, 		4(%ecx)
	movl %edi, 		8(%ecx)
	movl %ebp, 		12(%ecx)
	popl %edx
	movl %esp, 		16(%ecx)
	movl %edx, 		20(%ecx)
	xorl %eax, 		%eax
	jmp  *%edx

.global longjmp
longjmp:
	movl 8(%esp), 	%eax
	movl 4(%esp), 	%ecx
	movl 0(%ecx), 	%ebx
	movl 4(%ecx), 	%esi
	movl 8(%ecx), 	%edi
	movl 12(%ecx),	%ebp
	movl 16(%ecx), 	%esp
	orl	 %eax,		%eax
	jnz	 0f
	incl %eax
0:	jmp	 *20(%ecx)
