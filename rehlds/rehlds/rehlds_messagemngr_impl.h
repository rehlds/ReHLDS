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
*/

#pragma once

#include "IMessageManager.h"
#include "hookchains.h"

#include <memory> // std::unique_ptr
#include <algorithm> // std::move

/**
 * @brief Implementation interface manages hooks and blocking behavior game messages
 */
class MessageManagerImpl: public IMessageManager
{
public:

	void Init();

	MessageManagerImpl();
	~MessageManagerImpl() = default;

	/**
	 * @brief Returns the major version of the MessageManager
	 * @return The major version
	 */
	int getMajorVersion() const { return MESSAGEMNGR_VERSION_MAJOR; }

	/**
	 * @brief Returns the minor version of the MessageManager
	 * @return The minor version
	 */
	int getMinorVersion() const { return MESSAGEMNGR_VERSION_MINOR; }

	/**
	 * @brief Returns the blocking behavior for the given message type
	 * @param msg_id The message type
	 * @return The blocking behavior for the given message type
	 */
	IMessage::BlockType getMessageBlock(int msg_id) const;

	/**
	 * @brief Sets the blocking behavior for the given message type
	 * @param msg_id The message type
	 * @param blockType The blocking behavior to set
	 */
	void setMessageBlock(int msg_id, IMessage::BlockType blockType);

	/**
	 * @brief Registers a hook function for the given message type
	 * @param msg_id The message type to register the hook for
	 * @param handler The hook function to register
	 * @param priority The priority of the hook function (see enum HookChainPriority)
	 */
	void registerHook(int msg_id, hookfunc_t handler, int priority = HC_PRIORITY_DEFAULT);

	/**
	 * @brief Unregisters a hook function for the given message type
	 * @param msg_id The message type to unregister the hook for
	 * @param handler The hook function to unregister
	 */
	void unregisterHook(int msg_id, hookfunc_t handler);

private:
	friend void PF_MessageBegin_Intercept(int msg_dest, int msg_id, const float *pOrigin, edict_t *ed);
	friend void PF_MessageEnd_Intercept();
	friend void PF_WriteByte_Intercept(int iValue);
	friend void PF_WriteChar_Intercept(int iValue);
	friend void PF_WriteShort_Intercept(int iValue);
	friend void PF_WriteLong_Intercept(int iValue);
	friend void PF_WriteAngle_Intercept(float flValue);
	friend void PF_WriteCoord_Intercept(float flValue);
	friend void PF_WriteString_Intercept(const char *sz);
	friend void PF_WriteEntity_Intercept(int iValue);

	bool MessageBegin(int msg_dest, int msg_id, const float *pOrigin, edict_t *ed);
	bool MessageEnd();

private:
	bool WriteParam(IMessage::ParamType type, bool sign = false, size_t length = -1);

	bool m_inblock; // Flag indicating whether a message block is currently active
	bool m_inhook;  // Flag indicating whether a message hook is currently active

	/**
	 * @brief The fixed-size memory pool holds a list of free objects
	 * @tparam T The type of objects stored in the stack
	 * @tparam MAX The maximum size of the stack
	 */
	template <typename T, size_t MAX>
	class MessagePool
	{
	public:
		std::unique_ptr<T> acquire()
		{
			if (_size > 0)
				return std::move(_freeObjects[--_size]); // reusing
			return std::make_unique<T>(); // initialize constructor for new element
		}

		void release(std::unique_ptr<T> obj)
		{
			if (_size < MAX)
				_freeObjects[_size++] = std::move(obj);
		}

		void clear()
		{
			while (_size > 0)
				_freeObjects[--_size].reset();
		}

	private:
		size_t _size{0u};
		std::unique_ptr<T> _freeObjects[MAX]{};
	};

	/**
	 * @brief Helper a templated MessageStack class to manage a stack of Message objects with fixed size
	 * @tparam T The type of objects stored in the stack
	 * @tparam MAX The maximum size of the stack
	 */
	template <typename T, size_t MAX>
	class MessageStack
	{
	public:
		MessageStack(MessagePool<T, MAX> &pool) : _pool(pool) {}

		T &push()
		{
			std::unique_ptr<T> msg = _pool.acquire();
			_activeObjects[_size++] = std::move(msg);
			return *_activeObjects[_size - 1];
		}

		void pop()
		{
			if (_size > 0)
				_pool.release(std::move(_activeObjects[--_size]));
		}

		size_t size()     const { return _size; }
		size_t max_size() const { return MAX;   }

		const T &top() const { return *_activeObjects[_size - 1]; }
		      T &top()       { return *_activeObjects[_size - 1]; }

	private:
		size_t _size{0u};
		std::unique_ptr<T> _activeObjects[MAX]{};
		MessagePool<T, MAX> &_pool;
	};

	static const size_t MAX_MSGSTACK = 16; // The maximum size of the message stack, 16 it should be enough
	MessagePool<class MessageImpl, MAX_MSGSTACK> m_pool; // A fixed-size memory pool stack for reusable
	MessageStack<class MessageImpl, MAX_MSGSTACK> m_stack;

	using HookRegistry_t = IVoidHookChainRegistryImpl<IMessage *>;
	HookRegistry_t *m_hooks[MAX_USERMESSAGES]{};

	IMessage::BlockType m_blocks[MAX_USERMESSAGES]{}; // The array of blocking behaviors for each message type
};

/**
 * @brief The singleton instance of the MessageManager
 */
extern MessageManagerImpl &MessageManager();
