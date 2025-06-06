/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ByteBuffer.h"
#include "Errors.h"
#include "Log.h"
#include <utf8.h>
#include <algorithm>
#include <sstream>
#include <cmath>

ByteBufferPositionException::ByteBufferPositionException(size_t pos, size_t size, size_t valueSize)
    : ByteBufferException(Trinity::StringFormat("Attempted to get value with size: {} in ByteBuffer (pos: {} size: {})", valueSize, pos, size))
{
}

ByteBufferInvalidValueException::ByteBufferInvalidValueException(char const* type, std::string_view value)
    : ByteBufferException(Trinity::StringFormat("Invalid {} value ({}) found in ByteBuffer", type, value))
{
}

ByteBuffer& ByteBuffer::operator>>(float& value)
{
    read(&value, 1);
    if (!std::isfinite(value))
        throw ByteBufferInvalidValueException("float", "infinity");
    return *this;
}

ByteBuffer& ByteBuffer::operator>>(double& value)
{
    read(&value, 1);
    if (!std::isfinite(value))
        throw ByteBufferInvalidValueException("double", "infinity");
    return *this;
}

std::string_view ByteBuffer::ReadCString(bool requireValidUtf8 /*= true*/)
{
    if (_rpos >= size())
        throw ByteBufferPositionException(_rpos, 1, size());

    ResetBitPos();

    char const* begin = reinterpret_cast<char const*>(_storage.data()) + _rpos;
    char const* end = reinterpret_cast<char const*>(_storage.data()) + size();
    char const* stringEnd = std::ranges::find(begin, end, '\0');
    if (stringEnd == end)
        throw ByteBufferPositionException(size(), 1, size());

    std::string_view value(begin, stringEnd);
    _rpos += value.length() + 1;
    if (requireValidUtf8 && !utf8::is_valid(value.begin(), value.end()))
        throw ByteBufferInvalidValueException("string", value);
    return value;
}

std::string_view ByteBuffer::ReadString(uint32 length, bool requireValidUtf8 /*= true*/)
{
    if (_rpos + length > size())
        throw ByteBufferPositionException(_rpos, length, size());

    ResetBitPos();
    if (!length)
        return {};

    std::string_view value(reinterpret_cast<char const*>(&_storage[_rpos]), length);
    _rpos += length;
    if (requireValidUtf8 && !utf8::is_valid(value.begin(), value.end()))
        throw ByteBufferInvalidValueException("string", value);
    return value;
}

void ByteBuffer::append(uint8 const* src, size_t cnt)
{
    ASSERT(src, "Attempted to put a NULL-pointer in ByteBuffer (pos: " SZFMTD " size: " SZFMTD ")", _wpos, size());
    ASSERT(cnt, "Attempted to put a zero-sized value in ByteBuffer (pos: " SZFMTD " size: " SZFMTD ")", _wpos, size());
    ASSERT((size() + cnt) < 100000000);

    FlushBits();

    size_t const newSize = _wpos + cnt;
    if (_storage.capacity() < newSize) // custom memory allocation rules
    {
        if (newSize < 100)
            _storage.reserve(300);
        else if (newSize < 750)
            _storage.reserve(2500);
        else if (newSize < 6000)
            _storage.reserve(10000);
        else
            _storage.reserve(400000);
    }

    if (_storage.size() < newSize)
        _storage.resize(newSize);
    std::memcpy(&_storage[_wpos], src, cnt);
    _wpos = newSize;
}

void ByteBuffer::put(size_t pos, uint8 const* src, size_t cnt)
{
    ASSERT(pos + cnt <= size(), "Attempted to put value with size: " SZFMTD " in ByteBuffer (pos: " SZFMTD " size: " SZFMTD ")", cnt, pos, size());
    ASSERT(src, "Attempted to put a NULL-pointer in ByteBuffer (pos: " SZFMTD " size: " SZFMTD ")", pos, size());
    ASSERT(cnt, "Attempted to put a zero-sized value in ByteBuffer (pos: " SZFMTD " size: " SZFMTD ")", pos, size());

    std::memcpy(&_storage[pos], src, cnt);
}

void ByteBuffer::PutBits(std::size_t pos, std::size_t value, uint32 bitCount)
{
    ASSERT(pos + bitCount <= size() * 8, "Attempted to put %u bits in ByteBuffer (bitpos: " SZFMTD " size: " SZFMTD ")", bitCount, pos, size());
    ASSERT(bitCount, "Attempted to put a zero bits in ByteBuffer");

    for (uint32 i = 0; i < bitCount; ++i)
    {
        std::size_t wp = (pos + i) / 8;
        std::size_t bit = (pos + i) % 8;
        if ((value >> (bitCount - i - 1)) & 1)
            _storage[wp] |= 1 << (7 - bit);
        else
            _storage[wp] &= ~(1 << (7 - bit));
    }
}

void ByteBuffer::print_storage() const
{
    Logger const* networkLogger = sLog->GetEnabledLogger("network", LOG_LEVEL_TRACE);
    if (!networkLogger) // optimize disabled trace output
        return;

    std::ostringstream o;
    for (uint32 i = 0; i < size(); ++i)
        o << uint32(_storage[i]) << " - ";

    TC_LOG_TRACE("network", "STORAGE_SIZE: {} {}", size(), o.view());
}

void ByteBuffer::textlike() const
{
    Logger const* networkLogger = sLog->GetEnabledLogger("network", LOG_LEVEL_TRACE);
    if (networkLogger) // optimize disabled trace output
        return;

    std::ostringstream o;
    for (uint32 i = 0; i < size(); ++i)
        o << char(_storage[i]);

    sLog->OutMessageTo(networkLogger, "network", LOG_LEVEL_TRACE, "STORAGE_SIZE: {} {}", size(), o.view());
}

void ByteBuffer::hexlike() const
{
    Logger const* networkLogger = sLog->GetEnabledLogger("network", LOG_LEVEL_TRACE);
    if (!networkLogger) // optimize disabled trace output
        return;

    std::ostringstream o;
    o.setf(std::ios_base::hex, std::ios_base::basefield);
    o.fill('0');

    for (uint32 i = 0; i < size(); )
    {
        char const* sep = " | ";
        for (uint32 j = 0; j < 2; ++j)
        {
            for (uint32 k = 0; k < 8; ++k)
            {
                o.width(2);
                o << _storage[i];
                ++i;
            }

            o << sep;
            sep = "\n";
        }
    }

    sLog->OutMessageTo(networkLogger, "network", LOG_LEVEL_TRACE, "STORAGE_SIZE: {} {}", size(), o.view());
}

void ByteBuffer::OnInvalidPosition(size_t pos, size_t valueSize) const
{
    throw ByteBufferPositionException(pos, _storage.size(), valueSize);
}

template TC_SHARED_API char ByteBuffer::read<char>();
template TC_SHARED_API uint8 ByteBuffer::read<uint8>();
template TC_SHARED_API uint16 ByteBuffer::read<uint16>();
template TC_SHARED_API uint32 ByteBuffer::read<uint32>();
template TC_SHARED_API uint64 ByteBuffer::read<uint64>();
template TC_SHARED_API int8 ByteBuffer::read<int8>();
template TC_SHARED_API int16 ByteBuffer::read<int16>();
template TC_SHARED_API int32 ByteBuffer::read<int32>();
template TC_SHARED_API int64 ByteBuffer::read<int64>();
template TC_SHARED_API float ByteBuffer::read<float>();
template TC_SHARED_API double ByteBuffer::read<double>();
