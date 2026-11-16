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

IFileSystem *g_pFileSystem;

void FS_RemoveAllSearchPaths(void)
{
	g_pFileSystem->RemoveAllSearchPaths();
}

void FS_AddSearchPath(const char *pPath, const char *pathID)
{
	g_pFileSystem->AddSearchPath(pPath, pathID);
}

NOXREF int FS_RemoveSearchPath(const char *pPath)
{
	NOXREFCHECK;

	return g_pFileSystem->RemoveSearchPath(pPath);
}

void FS_RemoveFile(const char *pRelativePath, const char *pathID)
{
	g_pFileSystem->RemoveFile(pRelativePath, pathID);
}

void FS_CreateDirHierarchy(const char *path, const char *pathID)
{
	g_pFileSystem->CreateDirHierarchy(path, pathID);
}

int FS_FileExists(const char *pFileName)
{
	return g_pFileSystem->FileExists(pFileName);
}

NOXREF int FS_IsDirectory(const char *pFileName)
{
	NOXREFCHECK;

	return g_pFileSystem->IsDirectory(pFileName);
}

FileHandle_t FS_Open(const char *pFileName, const char *pOptions)
{
	return g_pFileSystem->Open(pFileName, pOptions, 0);
}

FileHandle_t FS_OpenPathID(const char *pFileName, const char *pOptions, const char *pathID)
{
	return g_pFileSystem->Open(pFileName, pOptions, pathID);
}

void FS_Close(FileHandle_t file)
{
	g_pFileSystem->Close(file);
}

void FS_Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType)
{
	g_pFileSystem->Seek(file, pos, seekType);
}

unsigned int FS_Tell(FileHandle_t file)
{
	return g_pFileSystem->Tell(file);
}

unsigned int FS_Size(FileHandle_t file)
{
	return g_pFileSystem->Size(file);
}

unsigned int FS_FileSize(const char *pFileName)
{
	return g_pFileSystem->Size(pFileName);
}

int32 FS_GetFileTime(const char *pFileName)
{
	return g_pFileSystem->GetFileTime(pFileName);
}

NOXREF void FS_FileTimeToString(char *pStrip, int maxCharsIncludingTerminator, int32 fileTime)
{
	NOXREFCHECK;

	g_pFileSystem->FileTimeToString(pStrip, maxCharsIncludingTerminator, fileTime);
}

int FS_IsOk(FileHandle_t file)
{
	return g_pFileSystem->IsOk(file);
}

void FS_Flush(FileHandle_t file)
{
	g_pFileSystem->Flush(file);
}

int FS_EndOfFile(FileHandle_t file)
{
	return g_pFileSystem->EndOfFile(file);
}

int FS_Read(void *pOutput, int size, int count, FileHandle_t file)
{
#ifdef REHLDS_FIXES
	return g_pFileSystem->Read(pOutput, size * count, file);
#else // REHLDS_FIXES
	return g_pFileSystem->Read(pOutput, size, file);
#endif // REHLDS_FIXES
}

int FS_Write(const void *pInput, int size, int count, FileHandle_t file)
{
#ifdef REHLDS_FIXES
	return g_pFileSystem->Write(pInput, size * count, file);
#else // REHLDS_FIXES
	return g_pFileSystem->Write(pInput, size, file);
#endif // REHLDS_FIXES
}

char *FS_ReadLine(char *pOutput, int maxChars, FileHandle_t file)
{
	return g_pFileSystem->ReadLine(pOutput, maxChars, file);
}

int FS_FPrintf(FileHandle_t file, char *pFormat, ...)
{
	char data[8192];
	va_list va;

	va_start(va, pFormat);
	Q_vsnprintf(data, sizeof(data), pFormat, va);
	data[sizeof(data) - 1] = 0;
	va_end(va);

	return g_pFileSystem->FPrintf(file, "%s", data);
}

const char *FS_FindFirst(const char *pWildCard, FileFindHandle_t *pHandle, const char *pathID)
{
	return g_pFileSystem->FindFirst(pWildCard, pHandle, pathID);
}

const char *FS_FindNext(FileFindHandle_t handle)
{
	return g_pFileSystem->FindNext(handle);
}

NOXREF int FS_FindIsDirectory(FileFindHandle_t handle)
{
	NOXREFCHECK;

	return g_pFileSystem->FindIsDirectory(handle);
}

void FS_FindClose(FileFindHandle_t handle)
{
	return g_pFileSystem->FindClose(handle);
}

void FS_GetLocalCopy(const char *pFileName)
{
	g_pFileSystem->GetLocalCopy(pFileName);
}

const char *FS_GetLocalPath(const char *pFileName, char *pLocalPath, int localPathBufferSize)
{
	return g_pFileSystem->GetLocalPath(pFileName, pLocalPath, localPathBufferSize);
}

NOXREF char *FS_ParseFile(char *pFileBytes, char *pToken, int *pWasQuoted)
{
	NOXREFCHECK;

	bool wasquoted;
	char *result = g_pFileSystem->ParseFile(pFileBytes, pToken, &wasquoted);

	if (pWasQuoted)
	{
		*pWasQuoted = wasquoted;
	}

	return result;
}

NOXREF int FS_FullPathToRelativePath(const char *pFullpath, char *pRelative)
{
	NOXREFCHECK;

	return g_pFileSystem->FullPathToRelativePath(pFullpath, pRelative);
}

NOXREF int FS_GetCurrentDirectory(char *pDirectory, int maxlen)
{
	NOXREFCHECK;

	return g_pFileSystem->GetCurrentDirectory(pDirectory, maxlen);
}

NOXREF void FS_PrintOpenedFiles(void)
{
	NOXREFCHECK;

	g_pFileSystem->PrintOpenedFiles();
}

NOXREF void FS_SetWarningFunc(void (*pfnWarning)(const char *, ...))
{
	NOXREFCHECK;

	g_pFileSystem->SetWarningFunc(pfnWarning);
}

NOXREF void FS_SetWarningLevel(FileWarningLevel_t level)
{
	NOXREFCHECK;

	g_pFileSystem->SetWarningLevel(level);
}

NOXREF unsigned char FS_GetCharacter(FileHandle_t f)
{
	NOXREFCHECK;

	uint8 retval;
	g_pFileSystem->Read(&retval, 1, f);
	return retval;
}

void FS_LogLevelLoadStarted(const char *name)
{
	g_pFileSystem->LogLevelLoadStarted(name);
}

void FS_LogLevelLoadFinished(const char *name)
{
	g_pFileSystem->LogLevelLoadFinished(name);
}

int FS_SetVBuf(FileHandle_t stream, char *buffer, int mode, size_t size)
{
	return g_pFileSystem->SetVBuf(stream, buffer, mode, size);
}

void FS_GetInterfaceVersion(char *p, int maxlen)
{
	g_pFileSystem->GetInterfaceVersion(p, maxlen);
}

void *FS_GetReadBuffer(FileHandle_t file, int *outBufSize)
{
	return g_pFileSystem->GetReadBuffer(file, outBufSize, 1);
}

void FS_ReleaseReadBuffer(FileHandle_t file, void *buffer)
{
	g_pFileSystem->ReleaseReadBuffer(file, buffer);
}

void FS_Unlink(const char *filename)
{
	char localPath[512];
	FS_GetLocalPath(filename, localPath, 512);
	_unlink(localPath);
}

void FS_Rename(const char *originalName, const char *newName)
{
	char *cut;
	char localPath[512];
	char newPath[512];

	if (FS_GetLocalPath(originalName, localPath, ARRAYSIZE(localPath)))
	{
		Q_strcpy(newPath, localPath);
		cut = Q_strstr(newPath, originalName);

		if (cut)
		{
			*cut = 0;
#ifdef REHLDS_CHECKS
			Q_strncat(newPath, newName, ARRAYSIZE(newPath) - Q_strlen(newPath));
			newPath[ARRAYSIZE(newPath) - 1] = 0;
#else
			Q_strcat(newPath, newName);
#endif

			rename(localPath, newPath);
		}
	}
}

void *FS_LoadLibrary(const char *dllName)
{
	void *result = NULL;

	if (dllName)
	{
		FS_GetLocalCopy(dllName);
#ifdef _WIN32
		result = LoadLibraryA(dllName);
#else
		result = dlopen(dllName, RTLD_NOW);
#endif
	}

	return result;
}
