#include "CriticalSectionDlg.h"

CriticalSectionDlg* CriticalSectionDlg::ptr = NULL;

CRITICAL_SECTION cs;

CriticalSectionDlg::CriticalSectionDlg(void)
{
    ptr = this;
}

CriticalSectionDlg::~CriticalSectionDlg(void)
{
    DeleteCriticalSection(&cs);
}

void CriticalSectionDlg::Cls_OnClose(HWND hwnd)
{
    EndDialog(hwnd, 0);
}

BOOL CriticalSectionDlg::Cls_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    InitializeCriticalSection(&cs);
    hEdit = GetDlgItem(hwnd, IDC_EDIT1);
    wofstream original(L"array.txt", ios::trunc);
    original << "HELLO WORLD!";
    original.close();
    return TRUE;
}

void MessageAboutError(DWORD dwError)
{
    LPVOID lpMsgBuf = NULL;
    TCHAR szBuf[300];

    BOOL fOK = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    if (lpMsgBuf != NULL)
    {
        wsprintf(szBuf, TEXT("Ошибка %d: %s"), dwError, lpMsgBuf);
        MessageBox(0, szBuf, TEXT("Сообщение об ошибке"), MB_OK | MB_ICONSTOP);
        LocalFree(lpMsgBuf);
    }
}

DWORD WINAPI WriteToFiles(LPVOID lp)
{
    EnterCriticalSection(&cs);
    HWND hEdit = HWND(lp);
    int len = GetWindowTextLength(hEdit);
    WCHAR* buff = new WCHAR[len + 1];
    GetWindowText(hEdit, buff, len + 1);
    int copies = wcstol(buff, nullptr, 10);
    delete[] buff;
    for (size_t i = 1; i <= copies; i++)
    {
        wifstream original(L"array.txt");
        wstringstream copyNameStream;
        copyNameStream << L"copy_" << i << L".txt";
        wstring copyName = copyNameStream.str();
        wofstream copy(copyName.c_str());
        copy << original.rdbuf();
        copy.close();
        original.close();
    }
    LeaveCriticalSection(&cs);

    MessageBox(0, TEXT("Копии созданы"), TEXT("Критическая секция"), MB_OK);
    return 0;
}

DWORD WINAPI ReadFromFiles(LPVOID lp)
{
    EnterCriticalSection(&cs);
    HWND hEdit = HWND(lp);
    int len = GetWindowTextLength(hEdit);
    WCHAR* buff = new WCHAR[len + 1];
    GetWindowText(hEdit, buff, len + 1);
    int copies = wcstol(buff, nullptr, 10);
    delete[] buff;

    wofstream out(L"array.txt", ios::app);
    if (!out.is_open())
    {
        MessageAboutError(GetLastError());
        LeaveCriticalSection(&cs);
        return 1;
    }

    wstring line;
    for (int i = 1; i <= copies; i++)
    {
        wstringstream copyNameStream;
        copyNameStream << L"copy_" << i << L".txt";
        wstring copyName = copyNameStream.str();
        wifstream copy(copyName.c_str());
        getline(copy, line);
        out << line << endl;
        copy.close();
    }
    out.close();

    LeaveCriticalSection(&cs);
    MessageBox(0, TEXT("Содержимое копий записано в оригинальный файл"), TEXT("Критическая секция"), MB_OK);
    return 0;
}

void CriticalSectionDlg::Cls_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (id == IDC_BUTTON1)
    {
        HANDLE hThread = CreateThread(NULL, 0, WriteToFiles, hEdit, 0, NULL);
        CloseHandle(hThread);
        hThread = CreateThread(NULL, 0, ReadFromFiles, hEdit, 0, NULL);
        CloseHandle(hThread);
    }
}

BOOL CALLBACK CriticalSectionDlg::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hwnd, WM_CLOSE, ptr->Cls_OnClose);
        HANDLE_MSG(hwnd, WM_INITDIALOG, ptr->Cls_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, ptr->Cls_OnCommand);
    }
    return FALSE;
}
