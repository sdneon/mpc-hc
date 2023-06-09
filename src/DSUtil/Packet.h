/*
 * (C) 2006-2020 see Authors.txt
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

#include <deque>
#include <mutex>
#include <vector>
#include <mpc_defines.h>

#define PACKET_AAC_RAW 0x0001

class CPacket : public std::vector<BYTE>
{
public:
	DWORD TrackNumber      = 0;
	BOOL bDiscontinuity    = FALSE;
	BOOL bSyncPoint        = FALSE;
	REFERENCE_TIME rtStart = INVALID_TIME;
	REFERENCE_TIME rtStop  = INVALID_TIME;
	AM_MEDIA_TYPE* pmt     = nullptr;

	DWORD Flag             = 0;

	virtual ~CPacket() {
		DeleteMediaType(pmt);
	}
	bool SetCount(const size_t newsize) {
		try {
			resize(newsize);
		}
		catch (...) {
			return false;
		}
		return true;
	}
	void SetData(const CPacket& packet) {
		*this = packet;
	}
	void SetData(const void* ptr, const size_t size) {
		resize(size);
		memcpy(data(), ptr, size);
	}
	void AppendData(const CPacket& packet) {
		insert(cend(), packet.cbegin(), packet.cend());
	}
	void AppendData(const void* ptr, const size_t size) {
		const size_t oldsize = this->size();
		resize(oldsize + size);
		memcpy(data() + oldsize, ptr, size);
	}
	void RemoveHead(const size_t size) {
		erase(begin(), begin() + size);
	}
};

class CPacketQueue2 : protected std::deque<CAutoPtr<CPacket>>
{
	size_t m_size = 0;
	std::mutex m_mutex;

public:
	void Add(CAutoPtr<CPacket> p);
	CAutoPtr<CPacket> Remove();
	void RemoveSafe(CAutoPtr<CPacket>& p, size_t& count);
	void RemoveAll();
	const size_t GetCount();
	const size_t GetSize();
	const REFERENCE_TIME GetDuration();
};
