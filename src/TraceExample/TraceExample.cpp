#include "stdafx.h"
#include "WPPGuid.h"
#include "TraceExample.tmh"

int _tmain()
{
    WPP_INIT_TRACING( L"TraceExample" );

    DoTraceMessage(TRACE_DEBUG, L"=============================================");
    DoTraceMessage(TRACE_DEBUG, L" Welcome to Etviewer Test application");
    DoTraceMessage(TRACE_DEBUG, L"=============================================");

    DoTraceMessage(TRACE_DEBUG, L"Test int %d", 12345);
    DoTraceMessage(TRACE_DEBUG, L"Test hex 0x%x", 0xABCD123);

    DoTraceMessage(TRACE_DEBUG, L"Test wide string message %ws", L"string");
    DoTraceMessage(TRACE_DEBUG, L"Test narrow string message %s", "string");

    DoTraceMessage(TRACE_DEBUG, L"This is a number %d", 1);

    DoTraceMessage(TRACE_DEBUG, L"Test special type PORT %!PORT!", 1);
    DoTraceMessage(TRACE_DEBUG, L"Test special type STATUS %!STATUS!", 1);
    DoTraceMessage(TRACE_DEBUG, L"Test special type WINERROR %!WINERROR!", 1);
    DoTraceMessage(TRACE_DEBUG, L"Test special type HRESULT %!HRESULT!", (HRESULT)1);
    DoTraceMessage(TRACE_DEBUG, L"Test special type NDIS_STATUS %!NDIS_STATUS!", 1);

    WPP_CLEANUP();
    return 0;
}

