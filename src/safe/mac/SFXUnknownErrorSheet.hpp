/*
 Safe: Encrypted File System
 Copyright (C) 2014 Rian Hunter <rian@alum.mit.edu>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef __safe_mac_SFXUnknownErrorSheet_hpp
#define __safe_mac_SFXUnknownErrorSheet_hpp

#import <safe/report_exception.hpp>
#import <safe/mac/last_throw_backtrace.hpp>

#import <stdexcept>

#import <Cocoa/Cocoa.h>

void
runUnknownErrorSheet(NSWindow *window,
                     NSString *title,
                     NSString *message,
                     safe::ExceptionLocation location,
                     const std::exception_ptr & eptr,
                     const safe::mac::Backtrace & offset_backtrace);

#endif
