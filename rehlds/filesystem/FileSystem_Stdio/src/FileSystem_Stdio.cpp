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

CFileSystem_Stdio::CFileSystem_Stdio()
{
	m_bMounted = false;
}

CFileSystem_Stdio::~CFileSystem_Stdio()
{
	RemoveAllSearchPaths();
	Trace_DumpUnclosedFiles();
}

void CFileSystem_Stdio::Mount()
{
	m_bMounted = true;
	CBaseFileSystem::Mount();
}

void CFileSystem_Stdio::Unmount()
{
	m_bMounted = false;
	CBaseFileSystem::Unmount();
}

// low-level filesystem wrapper
FILE *CFileSystem_Stdio::FS_fopen(const char *filename, const char *options, bool bFromCache)
{
	FILE *tst = fopen(filename, options);

#ifndef _WIN32
	if (!tst && !Q_strchr(options, 'w') && !Q_strchr(options, '+')) {
		const char *file = findFileInDirCaseInsensitive(filename);
		tst = fopen(file, options);
	}
#endif // _WIN32

	return tst;
}

void CFileSystem_Stdio::FS_fclose(FILE *fp)
{
	fclose(fp);
}

void CFileSystem_Stdio::FS_fseek(FILE *fp, int64_t pos, FileSystemSeek_t seekType)
{
	fseek(fp, pos, seekType);
}

size_t CFileSystem_Stdio::FS_ftell(FILE *fp)
{
	return ftell(fp);
}

int CFileSystem_Stdio::FS_feof(FILE *fp)
{
	return feof(fp);
}

size_t CFileSystem_Stdio::FS_fread(void *dest, size_t count, size_t size, FILE *fp)
{
	return fread(dest, count, size, fp);
}

size_t CFileSystem_Stdio::FS_fwrite(const void *src, size_t count, size_t size, FILE *fp)
{
	return fwrite(src, count, size, fp);
}

size_t CFileSystem_Stdio::FS_vfprintf(FILE *fp, const char *fmt, va_list list)
{
	return vfprintf(fp, fmt, list);
}

int CFileSystem_Stdio::FS_ferror(FILE *fp)
{
	return ferror(fp);
}

int CFileSystem_Stdio::FS_fflush(FILE *fp)
{
	return fflush(fp);
}

char *CFileSystem_Stdio::FS_fgets(char *dest, int destSize, FILE *fp)
{
	return fgets(dest, destSize, fp);
}

int CFileSystem_Stdio::FS_stat(const char *path, struct _stat *buf)
{
	int rt = _stat(path, buf);

#ifndef _WIN32
	if (rt == -1)
	{
		const char *file = findFileInDirCaseInsensitive(path);
		if (file) {
			rt = _stat(file, buf);
		}
	}
#endif // _WIN32

	return rt;
}

HANDLE CFileSystem_Stdio::FS_FindFirstFile(char *findname, WIN32_FIND_DATA *dat)
{
	return ::FindFirstFile(findname, dat);
}

bool CFileSystem_Stdio::FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA *dat)
{
	return (::FindNextFile(handle, dat) != 0);
}

bool CFileSystem_Stdio::FS_FindClose(HANDLE handle)
{
	return (::FindClose(handle) != 0);
}

bool CFileSystem_Stdio::IsThreadSafe()
{
	return true;
}

bool CFileSystem_Stdio::IsFileImmediatelyAvailable(const char *pFileName)
{
	return true;
}

void CFileSystem_Stdio::GetLocalCopy(const char *pFileName)
{
	;
}

void CFileSystem_Stdio::LogLevelLoadFinished(const char *name)
{
	;
}

void CFileSystem_Stdio::LogLevelLoadStarted(const char *name)
{
	;
}

int CFileSystem_Stdio::HintResourceNeed(const char *hintlist, int forgetEverything)
{
	return 0;
}

int CFileSystem_Stdio::PauseResourcePreloading()
{
	return 0;
}

int CFileSystem_Stdio::ResumeResourcePreloading()
{
	return 0;
}

int CFileSystem_Stdio::SetVBuf(FileHandle_t stream, char *buffer, int mode, long size)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(stream);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to SetVBuf NULL file handle!\n");
		return 0;
	}

	return setvbuf(fh->m_pFile, buffer, mode, size);
}

void CFileSystem_Stdio::GetInterfaceVersion(char *p, int maxlen)
{
	Q_strncpy(p, "Stdio", maxlen);
}

WaitForResourcesHandle_t CFileSystem_Stdio::WaitForResources(const char *resourcelist)
{
	return (WaitForResourcesHandle_t)FILESYSTEM_INVALID_HANDLE;
}

bool CFileSystem_Stdio::GetWaitForResourcesProgress(WaitForResourcesHandle_t handle, float *progress, bool *complete)
{
	if (progress) *progress = 0;
	if (complete) *complete = true;

	return false;
}

void CFileSystem_Stdio::CancelWaitForResources(WaitForResourcesHandle_t handle)
{
	;
}

bool CFileSystem_Stdio::IsAppReadyForOfflinePlay(int appID)
{
	return true;
}

EXPOSE_SINGLE_INTERFACE(CFileSystem_Stdio, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
