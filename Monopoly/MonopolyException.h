#pragma once

using namespace System;

namespace Monopoly
{
	private ref class MonopolyException : Exception
	{
	private:
		DWORD m_lastError;

	public:
		MonopolyException(LPCSTR message);
		MonopolyException(LPCSTR message, Exception^ inner);

		property String^ Message
		{
			String^ get() override
			{
				if (m_lastError != ERROR_SUCCESS)
				{
					LPTSTR lastErrorMessage;
					if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
						nullptr, m_lastError, 0, reinterpret_cast<LPTSTR>(&lastErrorMessage), 0, nullptr))
						return String::Format(gcnew String("{0} (LastError: {1})"), Exception::Message, gcnew String(lastErrorMessage));
				}

				return Exception::Message;
			}
		}
	};
}
