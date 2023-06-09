/*
 * (C) 2009-2020 see Authors.txt
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

#include "stdafx_common_cfg.h"
#include "stdafx_common.h"
#include <afxwin.h>                         // MFC core and standard components

 // Workaround compilation errors when including GDI+ with NOMINMAX defined
#include <algorithm>
namespace Gdiplus {
    using std::min;
    using std::max;
};

#include <afxbutton.h>

#include "stdafx_common_dshow.h"
#include <afxdlgs.h>


#include <algorithm>
#include <vector>
#include <list>

#include <dsound.h>
