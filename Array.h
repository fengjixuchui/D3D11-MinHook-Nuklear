#pragma once
#include <cstdint>

// Example usage.
//Array<Entity*> GetEntities()
//{
//	return *Memory::Ptr<Array<Entity*>*>(this, OFFSET_GAMEMANAGER_ENTITYLIST);
//}

namespace Engine
{
	template <typename T>
	class Array
	{
	private:
		T* m_pBuffer;
		uint64_t m_size;

	public:
		uint32_t GetSize()
		{
			return m_size;
		}

		const T& operator [](uint64_t i)
		{
			if (Memory::IsValidPtr<T>(m_pBuffer))
				return m_pBuffer[i];

			return nullptr;
		}
	};
}