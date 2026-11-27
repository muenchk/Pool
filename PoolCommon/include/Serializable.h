

#pragma once


#include <cstdint>
#include <shared_mutex>
#include <string>
#include <memory>
#include <set>
#include <map>
#include <unordered_map>
#include <list>


#include "Flags.h"
#include "BufferOperations.h"

namespace CommonLib
{
	class LoadResolver;
	class ISerializable;

	typedef uint64_t FormID;

	template <class T, typename = std::enable_if<std::is_base_of<ISerializable, T>::value>>
	struct FormIDLess
	{
		bool operator()(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs) const
		{
			if (!rhs)
				return true;
			return lhs->GetFormID() < rhs->GetFormID();
		}
	};

	typedef uint64_t EnumType;

	class ISerializable
	{
	private:
		static const uint32_t formversion = 0x1;

	public:
		struct SerializableFlags
		{
			enum Flag : EnumType
			{
				/// <summary>
				/// No flags
				/// </summary>
				None = 0 << 0,
				/// <summary>
				/// Do not free memory allocated by this instance
				/// </summary>
				DoNotFree = (EnumType)1 << 62,
				/// <summary>
				/// Form has been deleted
				/// </summary>
				Deleted = (EnumType)1 << 63,
			};
		};

	protected:
		FormID _formid = 0;
		Flags _flags;
		std::atomic_flag _flaglock = ATOMIC_FLAG_INIT;

		/// <summary>
		/// there have been changes to the forms values
		/// </summary>
		bool __changed = false;
		bool __saved = false;
		size_t __flaghash = 0;

		std::shared_mutex _lock;

	public:
		/// <summary>
		/// returns the total size of the fields with static size
		/// </summary>
		/// <returns></returns>
		virtual size_t GetDataSize();
		/// <summary>
		/// saves all relevant information of this instance to the given buffer
		/// </summary>
		/// <param name="buffer"></param>
		/// <returns></returns>
		virtual bool WriteData(Buffer::ArrayBuffer& buffer);
		/// <summary>
		/// reads all relevant information of this instance from the buffer
		/// </summary>
		/// <param name="buffer"></param>
		/// <param name="length"></param>
		virtual bool ReadData(Buffer::ArrayBuffer& buffer, LoadResolver* resolver) = 0;
		/// <summary>
		/// Early initialiazation pass [is executed once all forms have been loaded
		/// </summary>
		/// <param name="resolver"></param>
		virtual void InitializeEarly(LoadResolver* resolver) = 0;
		/// <summary>
		/// late initialization pass [is executed once all Initialize Early has been called for all forms]
		/// </summary>
		/// <param name="resolver"></param>
		virtual void InitializeLate(LoadResolver* resolver) = 0;
		/// <summary>
		/// Deletes all relevant for fields
		/// </summary>
		virtual void Delete() = 0;
		/// <summary>
		/// returns whether a form can be deleted
		/// </summary>
		/// <param name="data"></param>
		/// <returns></returns>
		virtual bool CanDelete() = 0;
		/// <summary>
		/// returns whether the form has been marked deleted
		/// </summary>
		/// <returns></returns>
		virtual bool IsDeleted()
		{
			return _flags.HasFlag(SerializableFlags::Deleted);
		}

		/// <summary>
		/// clear form internals
		/// </summary>
		virtual void ClearForm();

	protected:
		/// <summary>
		/// clear form internals
		/// </summary>
		virtual void ClearFormInternal();

	public:
		/// <summary>
		/// Clears all internals
		/// </summary>
		virtual void Clear() = 0;
		/// <summary>
		/// Returns the formtype
		/// </summary>
		/// <returns></returns>
		virtual int32_t GetType() = 0;
		/// <summary>
		/// Returns the formid of the instance
		/// </summary>
		/// <returns></returns>
		virtual FormID GetFormID();
		/// <summary>
		/// Sets the formid of the instance
		/// </summary>
		/// <param name="formid"></param>
		virtual void SetFormID(FormID formid);

		/// <summary>
		/// Attempts to release as much memory as possible
		/// </summary>
		virtual void FreeMemory() {};
		/// <summary>
		/// returns whether the memory of this form has been freed
		/// </summary>
		/// <returns></returns>
		virtual bool Freed() { return true; };
		/// <summary>
		/// returns an estimation of the memory used by this class
		/// </summary>
		/// <returns></returns>
		virtual size_t MemorySize() { return 0; }

		/// <summary>
		/// Acquires a writers lock on the instance
		/// </summary>
		void Lock();
		/// <summary>
		/// Tries to aqcuire a writers lock and return whether it has been optained
		/// </summary>
		bool TryLock();
		/// <summary>
		/// Aqcuires a readers lock on the instance
		/// </summary>
		void LockRead();
		/// <summary>
		/// Tries to aqcuire a readers lock and return whether it has been optained
		/// </summary>
		bool TryLockRead();
		/// <summary>
		/// Releases the writers lock on the instance
		/// </summary>
		void Unlock();
		/// <summary>
		/// Releases the readers lock on the instance
		/// </summary>
		void UnlockRead();

		virtual void SetFlag(EnumType flag);

		virtual void UnsetFlag(EnumType flag);

		virtual bool HasFlag(EnumType flag);

		virtual bool WasSaved();

		virtual void SetChanged();
		virtual void ClearChanged();
		virtual bool HasChanged();

		template <class T, typename = std::enable_if<std::is_base_of<ISerializable, T>::value>>
		T* As()
		{
			if (T::GetTypeStatic() == GetType())
				return dynamic_cast<T*>(this);
			else
				return nullptr;
		}

		template <class T, typename = std::enable_if<std::is_base_of<ISerializable, T>::value>>
		static std::string PrintForm(std::shared_ptr<T> form)
		{
			auto GetHex = [](uint64_t val) {
				std::stringstream ss;
				ss << std::hex << val;
				return ss.str();
			};
			if (!form)
				return "None";
			return std::string("[") + typeid(T).name() + "<" + GetHex(form->GetFormID()) + ">]";
		}


#pragma region Ref Counting
	public:
		void add_ref() const noexcept
		{
			count_.fetch_add(1, std::memory_order_relaxed);
		}

		void release() const noexcept
		{
			if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1)
				delete this;
		}

		std::size_t use_count() const noexcept
		{
			return count_.load(std::memory_order_relaxed);
		}

	protected:
		virtual ~ISerializable()
		{
			assert(count_.load(std::memory_order_relaxed) == 0);
		}

	private:
		mutable std::atomic<std::size_t> count_{ 0 };

	#pragma endregion
	};

	
	template <class... Ts>
	using enable_if_serializable_t = std::enable_if_t<(std::is_base_of<ISerializable, Ts>::value && ...), int>;

	

	
	template <class T>
	class Ref
	{
	public:
		using element_type = T;

		// ---- construction ----
		constexpr Ref() noexcept = default;
		constexpr Ref(std::nullptr_t) noexcept {}
		Ref(T* p) noexcept : ptr_(p)
		{
			if (ptr_)
				ptr_->add_ref();
		}  // Derived* -> Base* works implicitly

		Ref(const Ref& o) noexcept : ptr_(o.ptr_)
		{
			if (ptr_)
				ptr_->add_ref();
		}
		Ref(Ref&& o) noexcept : ptr_(o.detach()) {}

		// Converting copy/move: Ref<Derived> -> Ref<Base>, Ref<T> -> Ref<const T>
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		Ref(const Ref<U>& o) noexcept : ptr_(o.get())
		{
			if (ptr_)
				ptr_->add_ref();
		}

		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		Ref(Ref<U>&& o) noexcept : ptr_(o.detach())
		{}

		~Ref()
		{
			if (ptr_)
				ptr_->release();
		}

		// ---- assignment (by value: covers copy, move, converting, raw pointer, nullptr) ----
		Ref& operator=(Ref o) noexcept
		{
			swap(o);
			return *this;
		}

		// ---- modifiers ----
		void swap(Ref& o) noexcept { std::swap(ptr_, o.ptr_); }
		void reset() noexcept { Ref().swap(*this); }
		void reset(T* p) { Ref(p).swap(*this); }

		// Give up ownership WITHOUT release(); caller takes over the reference
		[[nodiscard]] T* detach() noexcept { return std::exchange(ptr_, nullptr); }
		// Take over an existing reference WITHOUT add_ref()
		[[nodiscard]] static Ref adopt(T* p) noexcept
		{
			Ref r;
			r.ptr_ = p;
			return r;
		}

		// ---- observers ----
		T* get() const noexcept { return ptr_; }
		T& operator*() const noexcept { return *ptr_; }
		T* operator->() const noexcept { return ptr_; }
		explicit operator bool() const noexcept { return ptr_ != nullptr; }
		std::size_t use_count() const noexcept { return ptr_ ? ptr_->use_count() : 0; }

	private:
		T* ptr_ = nullptr;
	};

	template <class T, enable_if_serializable_t<T> = 0>
	void swap(Ref<T>& a, Ref<T>& b) noexcept
	{
		a.swap(b);
	}

	// ---------------- casts (shared_ptr equivalents) ----------------
	template <typename T, typename U, enable_if_serializable_t<T, U> = 0>
	Ref<T> static_pointer_cast(const Ref<U>& r) noexcept
	{
		return Ref<T>(static_cast<T*>(r.get()));
	}
	template <typename T, typename U, enable_if_serializable_t<T, U> = 0>
	Ref<T> static_pointer_cast(Ref<U>&& r) noexcept
	{
		return Ref<T>::adopt(static_cast<T*>(r.detach()));
	}

	template <typename T, typename U, enable_if_serializable_t<T, U> = 0>
	Ref<T> const_pointer_cast(const Ref<U>& r) noexcept
	{
		return Ref<T>(const_cast<T*>(r.get()));
	}
	template <typename T, typename U, enable_if_serializable_t<T, U> = 0>
	Ref<T> const_pointer_cast(Ref<U>&& r) noexcept
	{
		return Ref<T>::adopt(const_cast<T*>(r.detach()));
	}

	// Returns an empty Ref on failure; the source keeps its reference (like shared_ptr)
	template <typename T, typename U, enable_if_serializable_t<T, U> = 0>
	Ref<T> dynamic_pointer_cast(const Ref<U>& r) noexcept
	{
		return Ref<T>(dynamic_cast<T*>(r.get()));
	}
	template <typename T, typename U, enable_if_serializable_t<T, U> = 0>
	Ref<T> dynamic_pointer_cast(Ref<U>&& r) noexcept
	{
		if (T* p = dynamic_cast<T*>(r.get())) {
			(void)r.detach();
			return Ref<T>::adopt(p);
		}
		return {};
	}

	// ---------------- comparisons: forward to the raw pointers ----------------
	// Ref <-> Ref (any pair whose pointers are comparable, e.g. Base/Derived)
	template <typename T, typename U, enable_if_serializable_t<T, U> = 0>
	bool operator==(const Ref<T>& a, const Ref<U>& b) noexcept
	{
		return a.get() == b.get();
	}
	template <typename T, typename U, enable_if_serializable_t<T, U> = 0>
	auto operator<=>(const Ref<T>& a, const Ref<U>& b) noexcept
	{
		return std::compare_three_way{}(a.get(), b.get());
	}

	// Ref <-> raw pointer (reversed order is synthesized by C++20)
	template <typename T, typename U, enable_if_serializable_t<T, U> = 0>
	bool operator==(const Ref<T>& a, U* b) noexcept
	{
		return a.get() == b;
	}
	template <typename T, typename U, enable_if_serializable_t<T, U> = 0>
	auto operator<=>(const Ref<T>& a, U* b) noexcept
	{
		return std::compare_three_way{}(a.get(), b);
	}

	// Ref <-> nullptr
	template <typename T, enable_if_serializable_t<T> = 0>
	bool operator==(const Ref<T>& a, std::nullptr_t) noexcept
	{
		return a.get() == nullptr;
	}
	template <typename T, enable_if_serializable_t<T> = 0>
	std::strong_ordering operator<=>(const Ref<T>& a, std::nullptr_t) noexcept
	{
		return std::compare_three_way{}(a.get(), static_cast<T*>(nullptr));
	}

	// ---------------- hashing ----------------
	template <class T>
	struct std::hash<CommonLib::Ref<T>>
	{
		std::size_t operator()(const CommonLib::Ref<T>& r) const noexcept { return std::hash<T*>()(r.get()); }
	};

	template <class T, enable_if_serializable_t<T> = 0>
	class FlagHolder
	{
		EnumType _flag;
		Ref<T> _serializable;

		FlagHolder(FlagHolder<T>&) = delete;
		FlagHolder(FlagHolder<T>&&) = delete;
		FlagHolder<T>& operator=(const FlagHolder<T>&) = delete;
		FlagHolder<T>& operator=(const FlagHolder<T>&&) = delete;

	public:
		FlagHolder()
		{
		}

		FlagHolder(Ref<T> serializable, EnumType flag)
		{
			if (serializable) {
				_serializable = serializable;
				_flag = flag;
				_serializable->SetFlag(flag);
			}
		}

		~FlagHolder()
		{
			if (_serializable) {
				_serializable->UnsetFlag(_flag);
				_serializable.reset();
			}
		}
	};

	template <class T, enable_if_serializable_t<T> = 0>
	class ReadLockHolder
	{
		Ref<T> _serializable;

		ReadLockHolder(ReadLockHolder<T>&) = delete;
		ReadLockHolder(ReadLockHolder<T>&&) = delete;
		ReadLockHolder<T>& operator=(const ReadLockHolder<T>&) = delete;
		ReadLockHolder<T>& operator=(const ReadLockHolder<T>&&) = delete;

	public:
		ReadLockHolder()
		{
		}

		ReadLockHolder(Ref<T> serializable)
		{
			if (serializable) {
				_serializable = serializable;
				_serializable->LockRead();
			}
		}

		~ReadLockHolder()
		{
			if (_serializable) {
				_serializable->UnlockRead();
				_serializable.reset();
			}
		}
	};

	template <class T, enable_if_serializable_t<T> = 0>
	class LockHolder
	{
		Ref<T> _serializable;

		LockHolder(LockHolder<T>&) = delete;
		LockHolder(LockHolder<T>&&) = delete;
		LockHolder<T>& operator=(const LockHolder<T>&) = delete;
		LockHolder<T>& operator=(const LockHolder<T>&&) = delete;

	public:
		LockHolder()
		{
		}

		LockHolder(Ref<T> serializable)
		{
			if (serializable) {
				_serializable = serializable;
				_serializable->Lock();
			}
		}

		~LockHolder()
		{
			if (_serializable) {
				_serializable->Unlock();
				_serializable.reset();
			}
		}
	};

	struct FormType
	{
		enum FormTypes
		{
			ExecutionHandler = 'EXEH',
			Execution = 'EXEC',
			TaskController = 'TASC',
			InputSingle = 'INPS',
			InputMulti = 'INPM',
		};
	};
}
