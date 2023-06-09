/*
 * (C) 2011-2021 see Authors.txt
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

CStringW GetFileOnly(LPCWSTR Path);
CStringW GetFolderOnly(LPCWSTR Path);
CStringW AddSlash(LPCWSTR Path);
CStringW RemoveSlash(LPCWSTR Path);
CStringW GetFileExt(LPCWSTR Path);
CStringW RenameFileExt(LPCWSTR Path, LPCWSTR Ext);
CStringW RemoveFileExt(LPCWSTR Path);
BOOL     GetTemporaryFilePath(CStringW strExtension, CStringW& strFileName);
CStringW CompactPath(LPCWSTR Path, UINT cchMax);

// Get path of specified module
CStringW GetModulePath(HMODULE hModule);
// Get path of the executable file of the current process
CStringW GetProgramPath();
// Get programm directory with slash
CStringW GetProgramDir();

// wFunc can be FO_MOVE or FO_COPY.
// To move a folder, add "\" to the end of the source path.
// To copy a folder, add "\*" to the end of the source path.
int FileOperation(const CStringW& source, const CStringW& target, const UINT wFunc);
