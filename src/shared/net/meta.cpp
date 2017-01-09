/*
   Drawpile - a collaborative drawing program.

   Copyright (C) 2013-2017 Calle Laakkonen

   Drawpile is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Drawpile is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Drawpile.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "meta.h"

#include <QtEndian>
#include <cstring>

namespace protocol {

UserJoin *UserJoin::deserialize(uint8_t ctx, const uchar *data, uint len)
{
	if(len<2)
		return nullptr;

	const uint8_t flags = data[0];
	const uint nameLen = data[1];

	// Name must be at least one character long, but hash is optional
	if(nameLen==0 || nameLen+2 > len)
		return nullptr;

	const QByteArray name = QByteArray((const char*)data+2, nameLen);
	const QByteArray hash = QByteArray((const char*)data+2+nameLen, len-2-nameLen);
	return new UserJoin(ctx, flags, name, hash);
}

int UserJoin::serializePayload(uchar *data) const
{
	uchar *ptr = data;
	*(ptr++) = m_flags;
	*(ptr++) = m_name.length();
	memcpy(ptr, m_name.constData(), m_name.length());
	ptr += m_name.length();
	memcpy(ptr, m_hash.constData(), m_hash.length());
	ptr += m_hash.length();

	return ptr - data;
}

int UserJoin::payloadLength() const
{
	return 1 + 1 + m_name.length() + m_hash.length();
}

SessionOwner *SessionOwner::deserialize(uint8_t ctx, const uchar *data, int len)
{
	if(len>255)
		return nullptr;

	QList<uint8_t> ids;
	ids.reserve(len);
	for(int i=0;i<len;++i)
		ids.append(data[i]);

	return new SessionOwner(ctx, ids);
}

int SessionOwner::serializePayload(uchar *data) const
{
	for(int i=0;i<m_ids.size();++i)
		data[i] = m_ids[i];
	return m_ids.size();
}

int SessionOwner::payloadLength() const
{
	return m_ids.size();
}

}
