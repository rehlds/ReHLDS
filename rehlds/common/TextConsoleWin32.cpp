/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "precompiled.h"

#if defined(_WIN32)

CTextConsoleWin32 console;
#pragma comment(lib, "user32.lib")

BOOL WINAPI ConsoleHandlerRoutine(DWORD CtrlType)
{
	// TODO ?
	/*if (CtrlType != CTRL_C_EVENT && CtrlType != CTRL_BREAK_EVENT)
	{
		// don't quit on break or ctrl+c
		m_System->Stop();
	}*/

	return TRUE;
}

// GetConsoleHwnd() helper function from MSDN Knowledge Base Article Q124103
// needed, because HWND GetConsoleWindow(VOID) is not avaliable under Win95/98/ME
HWND GetConsoleHwnd()
{
	HWND hwndFound;			// This is what is returned to the caller.
	char pszNewWindowTitle[1024];	// Contains fabricated WindowTitle
	char pszOldWindowTitle[1024];	// Contains original WindowTitle

	// Fetch current window title.
	GetConsoleTitle(pszOldWindowTitle, sizeof(pszOldWindowTitle));

	// Format a "unique" NewWindowTitle.
	wsprintf(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

	// Change current window title.
	SetConsoleTitle(pszNewWindowTitle);

	// Ensure window title has been updated.
	Sleep(40);

	// Look for NewWindowTitle.
	hwndFound = FindWindow(nullptr, pszNewWindowTitle);

	// Restore original window title.
	SetConsoleTitle(pszOldWindowTitle);

	return hwndFound;
}

CTextConsoleWin32::~CTextConsoleWin32()
{
	CTextConsoleWin32::ShutDown();
}

bool CTextConsoleWin32::Init(IBaseSystem *system)
{
	if (!AllocConsole())
		m_System = system;

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "[ReHLDS " APP_VERSION "] %s", m_System ? m_System->GetName() : "Console");
	SetTitle(buffer);
	//SetTitle(m_System ? m_System->GetName() : "Console");

	hinput = GetStdHandle(STD_INPUT_HANDLE);
	houtput = GetStdHandle(STD_OUTPUT_HANDLE);

	if (!SetConsoleCtrlHandler(&ConsoleHandlerRoutine, TRUE)) {
		Print("WARNING! TextConsole::Init: Could not attach console hook.\n");
	}

	Attrib = FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
	SetWindowPos(GetConsoleHwnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOREPOSITION | SWP_SHOWWINDOW);

	return CTextConsole::Init(system);
}

void CTextConsoleWin32::ShutDown()
{
	FreeConsole();
	CTextConsole::ShutDown();
}

void CTextConsoleWin32::SetVisible(bool visible)
{
	ShowWindow(GetConsoleHwnd(), visible ? SW_SHOW : SW_HIDE);
	m_ConsoleVisible = visible;
}

const char *CTextConsoleWin32::GetLine()
{
	while (true)
	{
		INPUT_RECORD recs[1024];
		unsigned long numread;
		unsigned long numevents;

		if (!GetNumberOfConsoleInputEvents(hinput, &numevents))
		{
			if (m_System) {
				m_System->Errorf("CTextConsoleWin32::GetLine: !GetNumberOfConsoleInputEvents\n");
			}

			return nullptr;
		}

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, ARRAYSIZE(recs), &numread))
		{
			if (m_System) {
				m_System->Errorf("CTextConsoleWin32::GetLine: !ReadConsoleInput\n");
			}

			return nullptr;
		}

		if (numread == 0)
			return nullptr;

		for (int i = 0; i < (int)numread; i++)
		{
			INPUT_RECORD *pRec = &recs[i];
			if (pRec->EventType != KEY_EVENT)
				continue;

			if (pRec->Event.KeyEvent.bKeyDown)
			{
				// check for cursor keys
				if (pRec->Event.KeyEvent.wVirtualKeyCode == VK_UP)
				{
					ReceiveUpArrow();
				}
				else if (pRec->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)
				{
					ReceiveDownArrow();
				}
				else if (pRec->Event.KeyEvent.wVirtualKeyCode == VK_LEFT)
				{
					ReceiveLeftArrow();
				}
				else if (pRec->Event.KeyEvent.wVirtualKeyCode == VK_RIGHT)
				{
					ReceiveRightArrow();
				}
				else
				{
					int nLen;
					char ch = pRec->Event.KeyEvent.uChar.AsciiChar;
					switch (ch)
					{
					case '\r':	// Enter
						nLen = ReceiveNewline();
						if (nLen)
						{
							return m_szConsoleText;
						}
						break;
					case '\b':	// Backspace
						ReceiveBackspace();
						break;
					case '\t':	// TAB
						ReceiveTab();
						break;
					default:
						// dont' accept nonprintable chars
						if ((ch >= ' ') && (ch <= '~'))
						{
							ReceiveStandardChar(ch);
						}
						break;
					}
				}
			}
		}
	}

	return nullptr;
}

void CTextConsoleWin32::PrintRaw(const char *pszMsg, int nChars)
{
#ifdef LAUNCHER_FIXES
	char outputStr[2048];
	WCHAR unicodeStr[1024];

	DWORD nSize = MultiByteToWideChar(CP_UTF8, 0, pszMsg, -1, NULL, 0);
	if (nSize > sizeof(unicodeStr))
		return;

	MultiByteToWideChar(CP_UTF8, 0, pszMsg, -1, unicodeStr, nSize);
	DWORD nLength = WideCharToMultiByte(CP_OEMCP, 0, unicodeStr, -1, 0, 0, NULL, NULL);
	if (nLength > sizeof(outputStr))
		return;

	WideCharToMultiByte(CP_OEMCP, 0, unicodeStr, -1, outputStr, nLength, NULL, NULL);
	WriteFile(houtput, outputStr, nChars ? nChars : Q_strlen(outputStr), NULL, NULL);
#else
	WriteFile(houtput, pszMsg, nChars ? nChars : Q_strlen(pszMsg), NULL, NULL);
#endif
}

void CTextConsoleWin32::Echo(const char *pszMsg, int nChars)
{
	PrintRaw(pszMsg, nChars);
}

int CTextConsoleWin32::GetWidth()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	int nWidth = 0;

	if (GetConsoleScreenBufferInfo(houtput, &csbi)) {
		nWidth = csbi.dwSize.X;
	}

	if (nWidth <= 1)
		nWidth = 80;

	return nWidth;
}

void CTextConsoleWin32::SetStatusLine(const char *pszStatus)
{
	Q_strncpy(statusline, pszStatus, sizeof(statusline) - 1);
	statusline[sizeof(statusline) - 2] = '\0';
	UpdateStatus();
}

void CTextConsoleWin32::UpdateStatus()
{
	COORD coord;
	DWORD dwWritten = 0;
	WORD wAttrib[ 80 ];

	for (int i = 0; i < 80; i++)
	{
		wAttrib[i] = Attrib; // FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
	}

	coord.X = coord.Y = 0;

	WriteConsoleOutputAttribute(houtput, wAttrib, 80, coord, &dwWritten);
	WriteConsoleOutputCharacter(houtput, statusline, 80, coord, &dwWritten);
}

void CTextConsoleWin32::SetTitle(const char *pszTitle)
{
	SetConsoleTitle(pszTitle);
}

void CTextConsoleWin32::SetColor(WORD attrib)
{
	Attrib = attrib;
}

#endif // defined(_WIN32)
