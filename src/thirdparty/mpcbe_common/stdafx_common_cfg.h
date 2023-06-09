/*
 * (C) 2009-2014 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#ifdef _WIN64
	#pragma warning(disable:4267) // hide warning C4267: conversion from 'size_t' to 'type', possible loss of data
#endif

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC // include Microsoft memory leak detection procedures
	#include <crtdbg.h>
	#define DNew DEBUG_NEW
#else
	#define DNew new
#endif
