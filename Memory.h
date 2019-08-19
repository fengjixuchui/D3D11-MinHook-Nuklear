#pragma once
#include <cstdint>
#include <type_traits>

namespace Engine
{
	class Memory
	{
	public:
		template <typename Type, typename Base, typename Offset>
		static inline Type Ptr(Base base, Offset offset)
		{
			static_assert(std::is_pointer<Type>::value || std::is_integral<Type>::value, "Type must be a pointer or address");
			static_assert(std::is_pointer<Base>::value || std::is_integral<Base>::value, "Base must be a pointer or address");
			static_assert(std::is_pointer<Offset>::value || std::is_integral<Offset>::value, "Offset must be a pointer or address");

			return base ? reinterpret_cast<Type>((reinterpret_cast<uint64_t>(base) + static_cast<uint64_t>(offset))) : nullptr;
		}

		template <typename Type>
		static bool IsValidPtr(Type* ptr)
		{
			return (ptr && sizeof(ptr)) ? true : false;
		}

		static bool IsValidPtr(void* ptr)
		{
			return (ptr && sizeof(ptr)) ? true : false;
		}
	};
}