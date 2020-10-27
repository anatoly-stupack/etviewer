#include "stdafx.h"
#include "WPPGuid.h"
#include "TraceExample.tmh"
#include <string>

int _tmain()
{
	WPP_INIT_TRACING(L"TraceExample");

	std::string extraLongString;

	for (int i = 0; i < 100; i++)
	{
		extraLongString += "1234567890";
	}

	wchar_t* string = L"test string";
	UNICODE_STRING unicodeString = { 0 };
	RtlInitUnicodeString(&unicodeString, string);

	DoTraceMessage(TRACE_DEBUG, "=============================================");
	DoTraceMessage(TRACE_DEBUG, " Welcome to Etviewer Test application");
	DoTraceMessage(TRACE_DEBUG, "=============================================");

	DoTraceMessage(TRACE_DEBUG, "------- Wide string formatting tests --------");
	DoTraceMessage(TRACE_DEBUG, L"Test int %d", 12345);
	DoTraceMessage(TRACE_DEBUG, L"Test 2 ints %d %d", 12345, 12345);
	DoTraceMessage(TRACE_DEBUG, L"Test hex 0x%x", 0xABCD123);
	DoTraceMessage(TRACE_DEBUG, L"Test qword %I64d", 12345);
	DoTraceMessage(TRACE_DEBUG, L"Test qword 0x%I64X", 0xABCD123);
	DoTraceMessage(TRACE_DEBUG, L"Test pointer 0x%p", (void*)0xABCD123);

	DoTraceMessage(TRACE_DEBUG, L"Test UNICODE_STRING %wZ", &unicodeString);
	DoTraceMessage(TRACE_DEBUG, L"Test 2 UNICODE_STRINGs %wZ %wZ", &unicodeString, &unicodeString);

	DoTraceMessage(TRACE_DEBUG, L"Test extra long string %s", extraLongString.c_str());

	DoTraceMessage(TRACE_DEBUG, L"Test wide string message %ws", L"string");
	DoTraceMessage(TRACE_DEBUG, L"Test narrow string message %s", "string");
	DoTraceMessage(TRACE_DEBUG, L"Test 2 narrow strings %s %s", "string 1", "string 2");

	DoTraceMessage(TRACE_DEBUG, L"Test special type PORT %!PORT!", 1);
	DoTraceMessage(TRACE_DEBUG, L"Test special type STATUS %!STATUS!", 1);
	DoTraceMessage(TRACE_DEBUG, L"Test special type WINERROR %!WINERROR!", 1);
	DoTraceMessage(TRACE_DEBUG, L"Test special type HRESULT %!HRESULT!", (HRESULT)1);
	DoTraceMessage(TRACE_DEBUG, L"Test special type NDIS_STATUS %!NDIS_STATUS!", 1);

	DoTraceMessage(TRACE_DEBUG, "------- Narrow string formatting tests --------");

	DoTraceMessage(TRACE_DEBUG, "Test int %d", 12345);
	DoTraceMessage(TRACE_DEBUG, "Test 2 ints %d %d", 12345, 12345);
	DoTraceMessage(TRACE_DEBUG, "Test hex 0x%x", 0xABCD123);
	DoTraceMessage(TRACE_DEBUG, "Test dec qword %I64d", 12345);
	DoTraceMessage(TRACE_DEBUG, "Test hex qword 0x%I64X", 0xABCD123);
	DoTraceMessage(TRACE_DEBUG, "Test pointer 0x%p", (void*)0xABCD123);

	DoTraceMessage(TRACE_DEBUG, "Test UNICODE_STRING %wZ", &unicodeString);
	DoTraceMessage(TRACE_DEBUG, "Test 2 UNICODE_STRINGs %wZ %wZ", &unicodeString, &unicodeString);

	DoTraceMessage(TRACE_DEBUG, "Test extra long string %s", extraLongString.c_str());

	DoTraceMessage(TRACE_DEBUG, "Test wide string message %ws", L"string");
	DoTraceMessage(TRACE_DEBUG, "Test narrow string message %s", "string");
	DoTraceMessage(TRACE_DEBUG, "Test 2 narrow strings %s %s", "string 1", "string 2");

	DoTraceMessage(TRACE_DEBUG, "Test special type PORT %!PORT!", 1);
	DoTraceMessage(TRACE_DEBUG, "Test special type STATUS %!STATUS!", 1);
	DoTraceMessage(TRACE_DEBUG, "Test special type WINERROR %!WINERROR!", 1);
	DoTraceMessage(TRACE_DEBUG, "Test special type HRESULT %!HRESULT!", (HRESULT)1);
	DoTraceMessage(TRACE_DEBUG, "Test special type NDIS_STATUS %!NDIS_STATUS!", 1);

	WPP_CLEANUP();
	return 0;
}