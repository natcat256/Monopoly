#include "pch.h"
#include "MonopolyException.h"

using namespace Monopoly;

MonopolyException::MonopolyException(LPCSTR message)
	: Exception(gcnew String(message)),
	m_lastError(GetLastError())
{
}

MonopolyException::MonopolyException(LPCSTR message, Exception^ inner)
	: Exception(gcnew String(message), inner),
	m_lastError(GetLastError())
{
}