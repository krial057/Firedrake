//
//  video.cpp
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

#include "video.h"
#include "textdevice.h"

namespace vd
{
	static video_device *_vd_activeDevice = nullptr;
	static text_device _vd_coreDevice;

	video_device::video_device()
	{
		backgroundColor = color::black;
		foregroundColor = color::light_gray;

		cursorX = cursorY = 0;

		pendingChange = false;
	}

	video_device::~video_device()
	{}

	bool video_device::interpret_character(char character)
	{
		if(character == 14 || character == 15)
		{
			pendingChange = true;
			foregroundChange = (character == 14);
			
			return true;
		}

		if(pendingChange && character >= 16 && character <= 31)
		{
			color nColor = static_cast<color>(character - 16);
			set_colors(foregroundChange ? nColor : get_foreground_color(), foregroundChange ? get_background_color() : nColor);

			return true;
		}

		return false;
	}



	void init()
	{
		_vd_activeDevice = &_vd_coreDevice;
	}

	video_device *get_active_device()
	{
		return _vd_activeDevice;
	}
}
