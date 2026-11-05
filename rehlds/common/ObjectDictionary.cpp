/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "precompiled.h"

ObjectDictionary::ObjectDictionary()
{
	m_currentEntry = 0;
	m_findKey = 0;
	m_entries = nullptr;

	Q_memset(m_cache, 0, sizeof(m_cache));

	m_cacheIndex = 0;
	m_size = 0;
	m_maxSize = 0;
}

ObjectDictionary::~ObjectDictionary()
{
	if (m_entries) {
		Mem_Free(m_entries);
	}
}

void ObjectDictionary::Clear(bool freeObjectssMemory)
{
	if (freeObjectssMemory)
	{
		for (int i = 0; i < m_size; i++)
		{
			void *obj = m_entries[i].object;
			if (obj) {
				Mem_Free(obj);
			}
		}
	}

	m_size = 0;
	CheckSize();
	ClearCache();
}

bool ObjectDictionary::Add(void *object, float key)
{
	if (m_size == m_maxSize && !CheckSize())
		return false;

	entry_t *p;
	if (m_size && key < m_entries[m_size - 1].key)
	{
		p = &m_entries[FindClosestAsIndex(key)];

		entry_t *e1 = &m_entries[m_size];
		entry_t *e2 = &m_entries[m_size - 1];

		while (p->key <= key) { p++; }
		while (p != e1)
		{
			e1->object = e2->object;
			e1->key = e2->key;

			e1--;
			e2--;
		}
	}
	else
		p = &m_entries[m_size];

	p->key = key;
	p->object = object;
	m_size++;

	ClearCache();
	AddToCache(p);

	return true;
}

int ObjectDictionary::FindClosestAsIndex(float key)
{
	if (m_size <= 0)
		return -1;

	if (key <= m_entries->key)
		return 0;

	int index = FindKeyInCache(key);
	if (index >= 0) {
		return index;
	}

	int middle;
	int first = 0;
	int last = m_size - 1;
	float keyMiddle;

	if (key < m_entries[last].key)
	{
		while (true)
		{
			middle = (last + first) >> 1;
			keyMiddle = m_entries[middle].key;

			if (keyMiddle == key)
				break;

			if (keyMiddle < key)
			{
				if (m_entries[middle + 1].key >= key)
				{
					if (m_entries[middle + 1].key - key < key - keyMiddle)
						++middle;
					break;
				}

				first = (last + first) >> 1;
			}
			else
			{
				last = (last + first) >> 1;
			}
		}
	}
	else
	{
		middle = last;
	}

	while (middle > 0 && m_entries[middle - 1].key == key) {
		--middle;
	}

	AddToCache(&m_entries[middle], key);
	return middle;
}
void ObjectDictionary::ClearCache()
{
	Q_memset(m_cache, 0, sizeof(m_cache));
	m_cacheIndex = 0;
}

bool ObjectDictionary::RemoveIndex(int index, bool freeObjectMemory)
{
	if (index < 0 || index >= m_size)
		return false;

	entry_t *p = &m_entries[m_size - 1];
	entry_t *e1 = &m_entries[index];
	entry_t *e2 = &m_entries[index + 1];

	if (freeObjectMemory && e1->object)
		Mem_Free(e1->object);

	while (p != e1)
	{
		e1->object = e2->object;
		e1->key = e2->key;

		e1++;
		e2++;
	}

	p->object = nullptr;
	p->key = 0;
	m_size--;

	CheckSize();
	ClearCache();

	return true;
}

bool ObjectDictionary::RemoveIndexRange(int minIndex, int maxIndex)
{
	if (minIndex > maxIndex)
	{
		if (maxIndex < 0)
			maxIndex = 0;

		if (minIndex >= m_size)
			minIndex = m_size - 1;
	}
	else
	{
		if (minIndex < 0)
			minIndex = 0;

		if (maxIndex >= m_size)
			maxIndex = m_size - 1;
	}

	int count = maxIndex - minIndex + 1;
	if (count <= 0 || minIndex >= m_size)
		return false;

	// Shift the remaining entries down over the removed range.
	int remaining = m_size - maxIndex - 1;
	if (remaining > 0) {
		Q_memmove(&m_entries[minIndex], &m_entries[maxIndex + 1], remaining * sizeof(entry_t));
	}

	m_size -= count;
	CheckSize();
	ClearCache();
	return true;
}

bool ObjectDictionary::Remove(void *object)
{
	bool found = false;
	for (int i = 0; i < m_size; i++)
	{
		if (m_entries[i].object == object) {
			RemoveIndex(i);
			found = true;
		}
	}

	return found ? true : false;
}

bool ObjectDictionary::RemoveSingle(void *object)
{
	for (int i = 0; i < m_size; i++)
	{
		if (m_entries[i].object == object) {
			RemoveIndex(i);
			return true;
		}
	}

	return false;
}

bool ObjectDictionary::RemoveKey(float key)
{
	int i = FindClosestAsIndex(key);
	if (i < 0 || m_entries[i].key != key)
	{
		return false;
	}

	int j = i;
	while (j + 1 < m_size && key == m_entries[j + 1].key) {
		++j;
	}

	return RemoveIndexRange(i, j);
}

bool ObjectDictionary::CheckSize()
{
	int newSize = m_maxSize;
	if (m_size == m_maxSize)
	{
		newSize = 1 - (int)(m_maxSize * -1.25f);
	}
	else if (m_maxSize * 0.5f > m_size)
	{
		newSize = (int)(m_maxSize * 0.75f);
	}

	if (newSize != m_maxSize)
	{
		entry_t *newEntries = (entry_t *)Mem_Malloc(sizeof(entry_t) * newSize);
		if (!newEntries)
			return false;

		Q_memset(&newEntries[m_size], 0, sizeof(entry_t) * (newSize - m_size));

		if (m_entries && m_size)
		{
			Q_memcpy(newEntries, m_entries, sizeof(entry_t) * m_size);
			Mem_Free(m_entries);
		}

		m_entries = newEntries;
		m_maxSize = newSize;
	}

	return true;
}

void ObjectDictionary::Init()
{
	m_size = 0;
	m_maxSize = 0;
	m_entries = nullptr;

	CheckSize();
	ClearCache();
}

void ObjectDictionary::Init(int baseSize)
{
	m_size = 0;
	m_maxSize = 0;
	m_entries = (entry_t *)Mem_ZeroMalloc(sizeof(entry_t) * baseSize);

	if (m_entries) {
		m_maxSize = baseSize;
	}
}

bool ObjectDictionary::Add(void *object)
{
	return Add(object, 0);
}

int ObjectDictionary::CountElements()
{
	return m_size;
}

bool ObjectDictionary::IsEmpty()
{
	return (m_size == 0) ? true : false;
}

bool ObjectDictionary::Contains(void *object)
{
	if (FindObjectInCache(object) >= 0)
		return true;

	for (int i = 0; i < m_size; i++)
	{
		entry_t *e = &m_entries[i];
		if (e->object == object) {
			AddToCache(e);
			return true;
		}
	}

	return false;
}

void *ObjectDictionary::GetFirst()
{
	m_currentEntry = 0;
	return GetNext();
}

void *ObjectDictionary::GetLast()
{
	return (m_size > 0) ? m_entries[m_size - 1].object : nullptr;
}

bool ObjectDictionary::ChangeKey(void *object, float newKey)
{
	int pos = FindObjectInCache(object);
	if (pos < 0)
	{
		for (pos = 0; pos < m_size; pos++)
		{
			if (m_entries[pos].object == object) {
				AddToCache(&m_entries[pos]);
				break;
			}
		}

		if (pos == m_size) {
			return false;
		}
	}

	entry_t *p, *e;

	p = &m_entries[pos];
	if (p->key == newKey)
		return false;

	int newpos = FindClosestAsIndex(newKey);
	e = &m_entries[newpos];
	if (pos < newpos)
	{
		if (e->key > newKey)
			e--;

		entry_t *e2 = &m_entries[pos + 1];
		while (p < e)
		{
			p->object = e2->object;
			p->key = e2->key;

			p++;
			e2++;
		}
	}
	else if (pos > newpos)
	{
		if (e->key > newKey)
			e++;

		entry_t *e2 = &m_entries[pos - 1];
		while (p > e)
		{
			p->object = e2->object;
			p->key = e2->key;

			p--;
			e2--;
		}
	}

	p->object = object;
	p->key = newKey;
	ClearCache();

	return true;
}

bool ObjectDictionary::UnsafeChangeKey(void *object, float newKey)
{
	int pos = FindObjectInCache(object);
	if (pos < 0)
	{
		for (pos = 0; pos < m_size; pos++)
		{
			if (m_entries[pos].object == object) {
				break;
			}
		}

		if (pos == m_size) {
			return false;
		}
	}

	m_entries[pos].key = newKey;
	ClearCache();
	return true;
}

void ObjectDictionary::AddToCache(entry_t *entry)
{
	int i = (m_cacheIndex % MAX_OBJECT_CACHE);

	m_cache[i].object = entry;
	m_cache[i].key = entry->key;
	m_cacheIndex++;
}

void ObjectDictionary::AddToCache(entry_t *entry, float key)
{
	int i = (m_cacheIndex % MAX_OBJECT_CACHE);

	m_cache[i].object = entry;
	m_cache[i].key = key;
	m_cacheIndex++;
}

int ObjectDictionary::FindKeyInCache(float key)
{
	for (auto& ch : m_cache)
	{
		if (ch.object && ch.key == key) {
			return (entry_t *)ch.object - m_entries;
		}
	}

	return -1;
}

int ObjectDictionary::FindObjectInCache(void *object)
{
	for (auto& ch : m_cache)
	{
		if (ch.object && ch.object == object) {
			return (entry_t *)ch.object - m_entries;
		}
	}

	return -1;
}

void *ObjectDictionary::FindClosestKey(float key)
{
	m_currentEntry = FindClosestAsIndex(key);
	return GetNext();
}

void *ObjectDictionary::GetNext()
{
	if (m_currentEntry < 0 || m_currentEntry >= m_size)
		return nullptr;

	return m_entries[m_currentEntry++].object;
}

void *ObjectDictionary::FindExactKey(float key)
{
	if ((m_currentEntry = FindClosestAsIndex(key)) < 0)
		return nullptr;

	return (m_entries[m_currentEntry].key == key) ? GetNext() : nullptr;
}
