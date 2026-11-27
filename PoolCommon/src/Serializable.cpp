#include "Serializable.h"

namespace CommonLib
{

	FormID ISerializable::GetFormID()
	{
		return _formid;
	}
	void ISerializable::SetFormID(FormID formid)
	{
		_formid = formid;
	}

	size_t ISerializable::GetDataSize(int32_t version)
	{
		size_t size_1 = 4     // version
		                + 8   // _formid
		                + 8;  // _flags

		size_1 += _flags.GetDataSize();

		return size_1;
	}

	bool ISerializable::WriteData(Buffer::ArrayBuffer& buffer)
	{
		SAVE_WRITE(buffer, uint32_t, formversion);
		SAVE_WRITE(buffer, FormID, _formid);
		SAVE_WRITE_EXPR(_flags.WriteData(buffer));

		__saved = true;

		return true;
	}

	bool ISerializable::ReadData(Buffer::ArrayBuffer& buffer, LoadResolver* /*resolver*/)
	{
		// sets saved to true -> form has been read from save
		__saved = true;
		uint32_t version = buffer.Read<uint32_t>();
		switch (version) {
		case 0x1:
			{
				_formid = buffer.Read<FormID>();
				_flags.ReadData(buffer);

				__flaghash = _flags.Hash();
			}
			return true;
		default:
			return false;
		}
	}

	bool ISerializable::TryLock()
	{
		return _lock.try_lock();
	}

	void ISerializable::Lock()
	{
		_lock.lock();
	}

	void ISerializable::LockRead()
	{
		_lock.lock_shared();
	}

	bool ISerializable::TryLockRead()
	{
		return _lock.try_lock_shared();
	}

	void ISerializable::Unlock()
	{
		_lock.unlock();
	}

	void ISerializable::UnlockRead()
	{
		_lock.unlock_shared();
	}
	
	void ISerializable::SetFlag(EnumType flag)
	{
		Spinlock guard(_flaglock);
		_flags.SetFlag(flag);
	}

	void ISerializable::UnsetFlag(EnumType flag)
	{
		Spinlock guard(_flaglock);
		_flags.UnsetFlag(flag);
	}

	bool ISerializable::HasFlag(EnumType flag)
	{
		return _flags.HasFlag(flag);
	}

	void ISerializable::SetChanged()
	{
		__changed |= 1;
	}
	void ISerializable::ClearChanged()
	{
		__changed ^= __changed;
		__flaghash = _flags.Hash();
	}
	bool ISerializable::HasChanged()
	{
		return __changed || __flaghash != _flags.Hash();
	}

	bool ISerializable::WasSaved()
	{
		return __saved;
	}

	void ISerializable::ClearForm()
	{
		Spinlock guard(_flaglock);
		_flags.Clear();
	}

	void ISerializable::ClearFormInternal()
	{
		_flags.Clear();
	}
}
