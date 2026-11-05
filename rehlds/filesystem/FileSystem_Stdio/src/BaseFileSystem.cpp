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

float g_flDummyFloat = 1.0f;
CBaseFileSystem *CBaseFileSystem::s_pFileSystem;

CBaseFileSystem *BaseFileSystem()
{
	return CBaseFileSystem::s_pFileSystem;
}

CBaseFileSystem::CBaseFileSystem()
{
	s_pFileSystem = this;

	m_fwLevel = FILESYSTEM_WARNING_REPORTUNCLOSED;
	m_pfnWarning = nullptr;
	m_nOpenCount = 0;
}

void CBaseFileSystem::Mount()
{
	g_flDummyFloat += g_flDummyFloat;
}

void CBaseFileSystem::Unmount()
{
	;
}

void CBaseFileSystem::CreateDirHierarchy(const char *path, const char *pathID)
{
	CSearchPath *searchPath = GetWritePath(pathID);
	if (!searchPath)
		return;

	int len = Q_strlen(searchPath->GetPath().String()) + Q_strlen(path);
	char *buf = (char *)alloca((len + 1) * sizeof(char));

	Q_strcpy(buf, searchPath->GetPath().String());
	Q_strcat(buf, path);

	FixSlashes(buf);

	char *s = buf;
	char *end = &buf[len];
	while (s < end)
	{
		if (*s == CORRECT_PATH_SEPARATOR) {
			*s = '\0';
#ifdef _WIN32
			_mkdir(buf);
#else
			mkdir(buf, S_IRWXU | S_IRGRP | S_IROTH); // owner has rwx, rest have r
#endif // _WIN32

			*s = CORRECT_PATH_SEPARATOR;
		}

		s++;
	}

#ifdef _WIN32
	_mkdir(buf);
#else
	mkdir(buf, S_IRWXU | S_IRGRP | S_IROTH); // owner has rwx, rest have r
#endif // _WIN32
}

void CBaseFileSystem::PrintOpenedFiles()
{
	Trace_DumpUnclosedFiles();
}

bool CBaseFileSystem::GetCurrentDirectory(char *pDirectory, int maxlen)
{
#ifdef _WIN32
	if (!::GetCurrentDirectoryA(maxlen, pDirectory))
#else
	if (!getcwd(pDirectory, maxlen))
#endif // _WIN32
		return false;

	FixSlashes(pDirectory);

	// Strip the last slash
	int len = Q_strlen(pDirectory);
	if (pDirectory[len - 1] == CORRECT_PATH_SEPARATOR) {
		pDirectory[len - 1] = '\0';
	}

	return true;
}

void CBaseFileSystem::AddSearchPathNoWrite(const char *pPath, const char *pathID)
{
	AddSearchPathInternal(pPath, pathID, false);
}

// Create the search path
void CBaseFileSystem::AddSearchPath(const char *pPath, const char *pathID)
{
	AddSearchPathInternal(pPath, pathID, true);
}

// This is where search paths are created. Map files are created at head of list
// (they occur after file system paths have already been set) so they get highest priority.
// Otherwise, we add the disk (non-packfile) path and then the paks if they exist for the path.
void CBaseFileSystem::AddSearchPathInternal(const char *pPath, const char *pathID, bool bAllowWrite)
{
	if (Q_strstr(pPath, ".bsp")) {
		return;
	}

	// Clean up the name
	char *newPath = (char *)alloca((MAX_PATH + 1) * sizeof(char));

#ifdef _WIN32
	if (Q_strchr(pPath, ':'))
#else
	if (pPath && *pPath == '/')
#endif // _WIN32
	{
		Q_strcpy(newPath, pPath);
	}
	else
	{
		GetCurrentDirectory(newPath, MAX_PATH);
		FixPath(newPath);

		if (Q_strcmp(pPath, ".") != 0) {
			Q_strcat(newPath, pPath);
		}
	}

#ifdef _WIN32 // don't do this on linux!
	Q_strlwr(newPath);
#endif // _WIN32

	FixPath(newPath);

	// Make sure that it doesn't already exist
	CUtlSymbol pathSymbol(newPath);
	CUtlSymbol pathIDSymbol(pathID);

	int c = m_SearchPaths.Count();
	for (int i = 0; i < c; i++)
	{
		CSearchPath *pSearchPath = &m_SearchPaths[i];
		if (pSearchPath->GetPath() == pathSymbol && pSearchPath->GetPathID() == pathIDSymbol) {
			// this entry is already at the head
			return;
		}
	}

	// Add to list
	int newIndex = m_SearchPaths.AddToTail();

	CSearchPath *sp = &m_SearchPaths[newIndex];
	sp->SetPath(pathSymbol);
	sp->SetPathID(pathIDSymbol);
	sp->SetWritePath(bAllowWrite);

	// Add pack files for this path next
	AddPackFiles(newPath);
}

// Returns true on success, false on failure
bool CBaseFileSystem::RemoveSearchPath(const char *pPath)
{
	char *newPath = (char *)alloca((MAX_PATH + Q_strlen(pPath) + 1) * sizeof(char));

#ifdef _WIN32
	if (Q_strchr(pPath, ':'))
#else
	if (pPath && *pPath == '/')
#endif // _WIN32
	{
		Q_strcpy(newPath, pPath);
	}
	else
	{
		GetCurrentDirectory(newPath, MAX_PATH);
		FixPath(newPath);
		if (Q_strcmp(pPath, ".")) {
			Q_strcat(newPath, pPath);
		}
	}

#ifdef _WIN32 // don't do this on linux!
	Q_strlwr(newPath);
#endif // _WIN32

	FixPath(newPath);

	CUtlSymbol lookup(newPath);

	bool bret = false;

	// Count backward since we're possibly deleting one or more pack files, too
	int i;
	int c = m_SearchPaths.Count();
	for (i = c - 1; i >= 0; i--)
	{
		if (m_SearchPaths[i].GetPath() != lookup)
			continue;

		m_SearchPaths.Remove(i);
		bret = true;
	}

	return bret;
}

// Finds a search path that should be used for writing to, given a pathID
CBaseFileSystem::CSearchPath *CBaseFileSystem::GetWritePath(const char *pathID)
{
	int iPath = 0;
	CSearchPath *searchPath = m_SearchPaths.Base();
	while (searchPath && iPath < m_SearchPaths.Count())
	{
		if (searchPath->IsAllowWrite()) {
			break;
		}

		searchPath = &m_SearchPaths[iPath++];
	}

	if (!pathID || m_SearchPaths.Count() <= 0) {
		return searchPath;
	}

	CUtlSymbol lookup(pathID);
	for (int i = 0; i < m_SearchPaths.Count(); i++)
	{
		if (m_SearchPaths[i].GetPathID() == lookup) {
			return &m_SearchPaths[i];
		}
	}

	return searchPath;
}

// Returns true on success, false on failure
bool CBaseFileSystem::IsDirectory(const char *pFileName)
{
	// Allow for UNC-type syntax to specify the path ID.
	struct _stat buf;

	for (int i = 0; i < m_SearchPaths.Count(); i++)
	{
		CSearchPath *sp = &m_SearchPaths[i];
		if (!sp->IsPackFile())
		{
			int len = Q_strlen(sp->GetPath().String()) + Q_strlen(pFileName);
			char *pTmpFileName = (char *)alloca((len + 1) * sizeof(char));
			Q_strcpy(pTmpFileName, sp->GetPath().String());
			Q_strcat(pTmpFileName, pFileName);

			FixSlashes(pTmpFileName);

			if (FS_stat(pTmpFileName, &buf) != -1)
			{
				if (buf.st_mode & _S_IFDIR)
					return true;
			}
		}
	}

	return false;
}

// The base file search goes through here
FileHandle_t CBaseFileSystem::FindFile(CSearchPath *path, const char *pFileName, const char *pOptions, bool bFromCache)
{
	CFileHandle *fh = nullptr;
	if (path->IsPackFile())
	{
		char *temp = (char *)alloca((Q_strlen(pFileName) + 1) * sizeof(char));

		Q_strcpy(temp, pFileName);
		Q_strlwr(temp);
		FixSlashes(temp);

		CPackFileEntry search;
		search.m_Name      = temp;
		search.m_nPosition = 0;
		search.m_nLength   = 0;

		int searchresult = path->m_PackFiles.Find(search);
		if (searchresult == path->m_PackFiles.InvalidIndex()) {
			return FILESYSTEM_INVALID_HANDLE;
		}

		CPackFileEntry result = path->m_PackFiles[searchresult];
		FS_fseek(path->m_hPackFile->m_pFile, path->m_hPackFile->m_nStartOffset + result.m_nPosition, FILESYSTEM_SEEK_HEAD);

		fh                  = new CFileHandle;
		fh->m_pFile         = path->m_hPackFile->m_pFile;
		fh->m_nStartOffset  = result.m_nPosition;
		fh->m_nLength       = result.m_nLength;
		fh->m_nFileTime     = path->m_lPackFileTime;
		fh->m_bPack         = true;
		fh->m_bErrorFlagged = false;
	}
	else if ((!Q_strchr(pOptions, 'w') && !Q_strchr(pOptions, '+')) || path->IsAllowWrite())
	{
		int len = Q_strlen(path->GetPath().String()) + Q_strlen(pFileName);
		char *pTmpFileName = (char *)alloca((len + 1) * sizeof(char));
		Q_strcpy(pTmpFileName, path->GetPath().String());
		Q_strcat(pTmpFileName, pFileName);

		FixSlashes(pTmpFileName);

		FILE *fp = Trace_FOpen(pTmpFileName, pOptions, bFromCache);
		if (!fp) {
			return FILESYSTEM_INVALID_HANDLE;
		}

		struct _stat buf = {};
		if (FS_stat(pTmpFileName, &buf) == -1) {
			Warning(FILESYSTEM_WARNING, "_stat on file %s which appeared to exist failed!!!\n", pTmpFileName);
		}

		fh                  = new CFileHandle;
		fh->m_pFile         = fp;
		fh->m_nStartOffset  = 0;
		fh->m_nLength       = buf.st_size;
		fh->m_nFileTime     = Q_max(buf.st_ctime, buf.st_mtime);
		fh->m_bPack         = false;
		fh->m_bErrorFlagged = false;
	}

	return reinterpret_cast<FileHandle_t>(fh);
}

// Returns true on success, false on failure.
bool CBaseFileSystem::FileExists(const char *pFileName)
{
	for (int i = 0; i < m_SearchPaths.Count(); i++)
	{
		int size = FastFindFileSize(&m_SearchPaths[i], pFileName);
		if (size != -1) {
			return true;
		}
	}

	return false;
}

FileHandle_t CBaseFileSystem::Open(const char *pFileName, const char *pOptions, const char *pathID)
{
	// Try each of the search paths in succession
	// FIXME: call createdirhierarchy upon opening for write.
	if (Q_strstr(pOptions, "r") && !Q_strstr(pOptions, "+"))
	{
		CUtlSymbol lookup(pathID);
		for (int i = 0; i < m_SearchPaths.Count(); i++)
		{
			if (pathID && m_SearchPaths[i].GetPathID() != lookup) {
				continue;
			}

			FileHandle_t filehandle = FindFile(&m_SearchPaths[i], pFileName, pOptions);
			if (filehandle != FILESYSTEM_INVALID_HANDLE) {
				return filehandle;
			}
		}

		return FILESYSTEM_INVALID_HANDLE;
	}

	CSearchPath *searchPath = GetWritePath(pathID);
	if (!searchPath) {
		return FILESYSTEM_INVALID_HANDLE;
	}

	int len = Q_strlen(searchPath->GetPath().String()) + Q_strlen(pFileName);
	char *pTmpFileName = (char *)alloca((len + 1) * sizeof(char));

	Q_strcpy(pTmpFileName, searchPath->GetPath().String());
	Q_strcat(pTmpFileName, pFileName);

	FixSlashes(pTmpFileName);

	FILE *fp = Trace_FOpen(pTmpFileName, pOptions);
	if (!fp) {
		return FILESYSTEM_INVALID_HANDLE;
	}

	struct _stat buf = {};
	if (FS_stat(pTmpFileName, &buf) == -1) {
		Warning(FILESYSTEM_WARNING, "_stat on file %s which appeared to exist failed!!!\n", pTmpFileName);
	}

	CFileHandle *fh;

	fh                  = new CFileHandle;
	fh->m_pFile         = fp;
	fh->m_nStartOffset  = 0;
	fh->m_nLength       = buf.st_size;
	fh->m_nFileTime     = buf.st_mtime;
	fh->m_bPack         = false;
	fh->m_bErrorFlagged = false;

	return reinterpret_cast<FileHandle_t>(fh);
}

FileHandle_t CBaseFileSystem::OpenFromCacheForRead(const char *pFileName, const char *pOptions, const char *pathID)
{
	CUtlSymbol lookup(pathID);
	for (int i = 0; i < m_SearchPaths.Count(); i++)
	{
		if (pathID && m_SearchPaths[i].GetPathID() != lookup) {
			continue;
		}

		FileHandle_t filehandle = FindFile(&m_SearchPaths[i], pFileName, pOptions, true);
		if (filehandle != FILESYSTEM_INVALID_HANDLE) {
			return filehandle;
		}
	}

	return FILESYSTEM_INVALID_HANDLE;
}

void CBaseFileSystem::Close(FileHandle_t file)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Close NULL file handle!\n");
		return;
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Close NULL file pointer inside valid file handle!\n");
		return;
	}

	// found handle in packfiles, we shouldn't release handle
	if (m_PackFileHandles.Find(fh->m_pFile) != m_PackFileHandles.InvalidIndex()) {
		return;
	}

	Trace_FClose(fh->m_pFile);
	fh->m_pFile = nullptr;
	delete fh;
}

void CBaseFileSystem::Seek(FileHandle_t file, int pos, FileSystemSeek_t whence)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "Tried to Seek NULL file handle!\n");
		return;
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Seek NULL file pointer inside valid file handle!\n");
		return;
	}

	// check bounds
	FileSystemSeek_t seekType = whence;
	if (seekType < FILESYSTEM_SEEK_HEAD || seekType > FILESYSTEM_SEEK_TAIL)
		seekType = FILESYSTEM_SEEK_HEAD;

	if (!fh->m_bPack)
	{
		FS_fseek(fh->m_pFile, pos, seekType);
		return;
	}

	if (whence == FILESYSTEM_SEEK_CURRENT)
	{
		FS_fseek(fh->m_pFile, pos, seekType);
		return;
	}

	if (whence == FILESYSTEM_SEEK_HEAD)
	{
		FS_fseek(fh->m_pFile, pos + fh->m_nStartOffset, seekType);
		return;
	}

	FS_fseek(fh->m_pFile, pos + fh->m_nStartOffset + fh->m_nLength, seekType);
}

size_t CBaseFileSystem::Tell(FileHandle_t file)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Tell NULL file handle!\n");
		return 0;
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Tell NULL file pointer inside valid file handle!\n");
		return 0;
	}

	// Pack files are relative
	return FS_ftell(fh->m_pFile) - (size_t)fh->m_nStartOffset;
}

size_t CBaseFileSystem::Size(FileHandle_t file)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Size NULL file handle!\n");
		return 0;
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Size NULL file pointer inside valid file handle!\n");
		return 0;
	}

	return fh->m_nLength;
}

size_t CBaseFileSystem::Size(const char *pFileName)
{
	int i = 0;
	while (i < m_SearchPaths.Count())
	{
		int size = FastFindFileSize(&m_SearchPaths[i], pFileName);
		if (size != -1) {
			return size;
		}

		i++;
	}

	return -1;
}

int CBaseFileSystem::FastFindFileSize(CSearchPath *path, const char *pFileName)
{
	if (path->IsPackFile())
	{
		char *temp = (char *)alloca((Q_strlen(pFileName) + 1) * sizeof(char));

		Q_strcpy(temp, pFileName);
		Q_strlwr(temp);
		FixSlashes(temp);

		CPackFileEntry search;
		search.m_Name      = temp;
		search.m_nPosition = 0;
		search.m_nLength   = 0;

		int searchresult = path->m_PackFiles.Find(search);
		if (searchresult == path->m_PackFiles.InvalidIndex()) {
			return -1;
		}

		return path->m_PackFiles[searchresult].m_nLength;
	}

	// Is it an absolute path?
	char pTmpFileName[MAX_PATH];
	Q_strlcpy(pTmpFileName, path->GetPath().String());
	Q_strlcat(pTmpFileName, pFileName);
	FixSlashes(pTmpFileName);

	struct _stat buf;
	if (FS_stat(pTmpFileName, &buf) == -1) {
		return -1;
	}

	return buf.st_size;
}

bool CBaseFileSystem::EndOfFile(FileHandle_t file)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to EndOfFile NULL file handle!\n");
		return true;
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to EndOfFile NULL file pointer inside valid file handle!\n");
		return true;
	}

	if (fh->m_bPack)
	{
		if (FS_ftell(fh->m_pFile) >= (fh->m_nStartOffset + fh->m_nLength)) {
			return true;
		}

		return false;
	}

	if (FS_feof(fh->m_pFile)) {
		return true;
	}

	return false;
}

int CBaseFileSystem::Read(void *pOutput, int size, FileHandle_t file)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Read NULL file handle!\n");
		return 0;
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Read NULL file pointer inside valid file handle!\n");
		return 0;
	}

	int result = FS_fread(pOutput, 1, size, fh->m_pFile);
	if (result != size) {
		fh->m_bErrorFlagged = true;
	}

	return result;
}

int CBaseFileSystem::Write(const void *pInput, int size, FileHandle_t file)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Write NULL file handle!\n");
		return 0;
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Write NULL file pointer inside valid file handle!\n");
		return 0;
	}

	return FS_fwrite(pInput, 1, size, fh->m_pFile);
}

int CBaseFileSystem::FPrintf(FileHandle_t file, const char *fmt, ...)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to FS: fprintf NULL file handle!\n");
		return 0;
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to FS: fprintf NULL file pointer inside valid file handle!\n");
		return 0;
	}

	va_list argptr;
	va_start(argptr, fmt);
	int len = FS_vfprintf(fh->m_pFile, fmt, argptr);
	va_end(argptr);

	return len;
}

void *CBaseFileSystem::GetReadBuffer(FileHandle_t file, int *outBufferSize, bool failIfNotInCache)
{
	*outBufferSize = 0;
	return nullptr;
}

void CBaseFileSystem::ReleaseReadBuffer(FileHandle_t file, void *readBuffer)
{
	;
}

bool CBaseFileSystem::IsOk(FileHandle_t file)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to IsOk NULL file handle!\n");
		return false;
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to IsOk NULL file pointer inside valid file handle!\n");
		return false;
	}

	if (fh->m_bErrorFlagged) {
		return false;
	}

	return FS_ferror(fh->m_pFile) == 0 ? true : false;
}

void CBaseFileSystem::Flush(FileHandle_t file)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Flush NULL file handle!\n");
		return;
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to Flush NULL file pointer inside valid file handle!\n");
		return;
	}

	FS_fflush(fh->m_pFile);
}

char *CBaseFileSystem::ReadLine(char *pOutput, int maxChars, FileHandle_t file)
{
	CFileHandle *fh = reinterpret_cast<CFileHandle *>(file);
	if (!fh) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to ReadLine NULL file handle!\n");
		return "";
	}

	if (!fh->m_pFile) {
		Warning(FILESYSTEM_WARNING, "FS:  Tried to ReadLine NULL file pointer inside valid file handle!\n");
		return "";
	}

	return FS_fgets(pOutput, maxChars, fh->m_pFile);
}

// Deletes a file
void CBaseFileSystem::RemoveFile(const char *pRelativePath, const char *pathID)
{
	CSearchPath *searchPath = GetWritePath(pathID);
	if (!searchPath)
		return;

	int len = Q_strlen(searchPath->GetPath().String()) + Q_strlen(pRelativePath);
	char *pTmpFileName = (char *)alloca((len + 1) * sizeof(char));

	Q_strcpy(pTmpFileName, searchPath->GetPath().String());
	Q_strcat(pTmpFileName, pRelativePath);

	FixSlashes(pTmpFileName);
	_unlink(pTmpFileName);
}

// Converts a partial path into a full path
// Relative paths that are pack based are returned as an absolute path .../zip?.zip/foo
// A pack absolute path can be sent back in for opening, and the file will be properly
// detected as pack based and mounted inside the pack.
const char *CBaseFileSystem::GetLocalPath(const char *pFileName, char *pLocalPath, int localPathBufferSize)
{
#ifdef _WIN32
	if (Q_strchr(pFileName, ':'))
#else
	if (pFileName && *pFileName == '/')
#endif // _WIN32
	{
		Q_strncpy(pLocalPath, pFileName, localPathBufferSize);
		pLocalPath[localPathBufferSize - 1] = '\0';
		FixSlashes(pLocalPath);
		return pLocalPath;
	}

	struct _stat buf;
	for (int i = 0; i < m_SearchPaths.Count(); i++)
	{
		CSearchPath *pSearchPath = &m_SearchPaths[i];
		if (pSearchPath->IsPackFile())
		{
			char *temp = (char *)alloca((Q_strlen(pFileName) + 1) * sizeof(char));

			Q_strcpy(temp, pFileName);
			Q_strlwr(temp);
			FixSlashes(temp);

			CPackFileEntry search;
			search.m_Name      = temp;
			search.m_nPosition = 0;
			search.m_nLength   = 0;

			int searchresult = pSearchPath->m_PackFiles.Find(search);
			if (searchresult != pSearchPath->m_PackFiles.InvalidIndex())
			{
				int len = Q_strlen(pFileName);
				char *pTmpFileName = (char *)alloca((len + 1) * sizeof(char));
				Q_strcpy(pTmpFileName, pFileName);

				Q_snprintf(pLocalPath, localPathBufferSize - 1, "%s%s", pSearchPath->GetPath().String(), pTmpFileName);
				pLocalPath[localPathBufferSize - 1] = '\0';
				FixSlashes(pLocalPath);
				return pLocalPath;
			}
		}
		else
		{
			int len = Q_strlen(pSearchPath->GetPath().String()) + Q_strlen(pFileName);
			char *pTmpFileName = (char *)alloca((len + 1) * sizeof(char));
			Q_strcpy(pTmpFileName, pSearchPath->GetPath().String());
			Q_strcat(pTmpFileName, pFileName);
			FixSlashes(pTmpFileName);

			if (FS_stat(pTmpFileName, &buf) != -1) {
				Q_strncpy(pLocalPath, pTmpFileName, localPathBufferSize);
				pLocalPath[localPathBufferSize - 1] = '\0';
				return pLocalPath;
			}
		}
	}

	return nullptr;
}

FILE *CBaseFileSystem::Trace_FOpen(const char *filename, const char *options, bool bFromCache)
{
	FILE *fp = FS_fopen(filename, options, bFromCache);
	if (!fp) {
		return nullptr;
	}

	COpenedFile file;
	file.SetName(filename);
	file.m_pFile = fp;

	m_OpenedFiles.AddToTail(file);
	m_nOpenCount++;

	if (m_fwLevel >= FILESYSTEM_WARNING_REPORTALLACCESSES) {
		Warning(FILESYSTEM_WARNING_REPORTALLACCESSES, "---FS:  open %s %p %i\n", filename, fp, m_nOpenCount);
	}

	return fp;
}

void CBaseFileSystem::Trace_FClose(FILE *fp)
{
	if (!fp) {
		return;
	}

	COpenedFile file;
	file.m_pFile = fp;

	int result = m_OpenedFiles.Find(file);
	if (result != m_OpenedFiles.InvalidIndex())
	{
		m_nOpenCount--;

		COpenedFile found = m_OpenedFiles[result];
		if (m_fwLevel >= FILESYSTEM_WARNING_REPORTALLACCESSES) {
			Warning(FILESYSTEM_WARNING_REPORTALLACCESSES, "---FS:  close %s %p %i\n", found.GetName(), fp, m_nOpenCount);
		}

		m_OpenedFiles.FindAndRemove(found);
	}
	else
	{
		Assert(0);

		if (m_fwLevel >= FILESYSTEM_WARNING_REPORTALLACCESSES)
		{
			Warning(FILESYSTEM_WARNING_REPORTALLACCESSES, "Tried to close unknown file pointer %p\n", fp);
		}
	}

	FS_fclose(fp);
}

void CBaseFileSystem::Trace_DumpUnclosedFiles()
{
	for (int i = 0; i < m_OpenedFiles.Count(); i++)
	{
		if (m_fwLevel <= FILESYSTEM_WARNING_QUIET) {
			continue;
		}

		COpenedFile *found = &m_OpenedFiles[i];
		Warning(FILESYSTEM_WARNING_REPORTUNCLOSED, "File %s was never closed\n", found ? found->GetName() : "???");
	}
}

bool CBaseFileSystem::AddPackFile(const char *pFileName, const char *pathID)
{
	return AddPackFileFromPath("", pFileName, true, pathID);
}

// Adds a pack file from the specified path
// Returns true on success, false on failure
bool CBaseFileSystem::AddPackFileFromPath(const char *pPath, const char *pakfile, bool bCheckForAppendedPack, const char *pathID)
{
	char fullpath[MAX_PATH];
	Q_snprintf(fullpath, sizeof(fullpath), "%s%s", pPath, pakfile);
	FixSlashes(fullpath);

	struct _stat buf;
	if (FS_stat(fullpath, &buf) == -1) {
		return false;
	}

	CFileHandle *fh = new CFileHandle;
	fh->m_nStartOffset  = 0;
	fh->m_nLength       = 0;
	fh->m_nFileTime     = 0;
	fh->m_bPack         = false;
	fh->m_bErrorFlagged = false;
	fh->m_pFile         = Trace_FOpen(fullpath, "rb");

	// Add this pack file to the search path
	int newIndex = m_SearchPaths.AddToTail();
	CSearchPath *sp = &m_SearchPaths[newIndex];
	sp->m_hPackFile     = fh;
	sp->m_Path          = pPath;
	sp->m_PathID        = pathID;
	sp->m_bIsPackFile   = true;
	sp->m_lPackFileTime = GetFileTime(pakfile);

	int64_t headeroffset = 0LL;

	// Get the length of the pack file
	if (bCheckForAppendedPack)
	{
		// Get the length of the pack file:
		packappenededheader_t appended;
		FS_fseek(fh->m_pFile, -signed(sizeof(appended)), FILESYSTEM_SEEK_TAIL);
		FS_fread(&appended, 1, sizeof(appended), fh->m_pFile);

		if (Q_strcmp(appended.id, "PACKAPPE") != 0)
		{
			m_SearchPaths.Remove(newIndex);
			return false;
		}

		headeroffset = appended.packheaderpos;
	}

	if (!PreparePackFile(*sp, headeroffset))
	{
		// Failed for some reason, ignore it
		Trace_FClose(fh->m_pFile);
		m_SearchPaths.Remove(newIndex);
		return false;
	}

	// Add handle to list
	m_PackFileHandles.AddToTail(fh->m_pFile);
	return true;
}

const size_t MAX_DIR_PACK_FILES = 0x8000u;

// Parse the pack file to build the file directory and preload section
bool CBaseFileSystem::PreparePackFile(CSearchPath &packfile, int64_t offsetofpackinmetafile)
{
	packheader_t header;
	FS_fseek(packfile.m_hPackFile->m_pFile, packfile.m_hPackFile->m_nStartOffset + offsetofpackinmetafile, FILESYSTEM_SEEK_HEAD);
	FS_fread(&header, 1, sizeof(header), packfile.m_hPackFile->m_pFile);

	if (*(uint32_t *)header.id == MAKEID('P', 'K', '6', '4')) {
		return Prepare64BitPackFile(packfile, offsetofpackinmetafile);
	}

	if (*(uint32_t *)header.id != MAKEID('P', 'A', 'C', 'K'))
	{
		Warning(FILESYSTEM_WARNING, "%s is not a packfile\n", packfile.GetPath().String());
		return false;
	}

	int numpackfiles = header.dirlen / sizeof(packfile_t);
	if (numpackfiles > MAX_DIR_PACK_FILES) {
		Warning(FILESYSTEM_WARNING, "%s has %i files\n", packfile.GetPath().String(), numpackfiles);
		return false;
	}

	if (numpackfiles > 0)
	{
		packfile_t *newfiles = new packfile_t[numpackfiles];
		if (!newfiles) {
			Warning(FILESYSTEM_WARNING, "%s out of memory allocating directory for %i files\n", packfile.GetPath().String(), numpackfiles);
			return false;
		}

		FS_fseek(packfile.m_hPackFile->m_pFile, packfile.m_hPackFile->m_nStartOffset + header.dirofs + offsetofpackinmetafile, FILESYSTEM_SEEK_HEAD);
		FS_fread(newfiles, 1, header.dirlen, packfile.m_hPackFile->m_pFile);

		for (int i = 0; i < numpackfiles; i++)
		{
			Q_strlwr(newfiles[i].name);
			FixSlashes(newfiles[i].name);

			CPackFileEntry lookup;
			lookup.m_Name      = newfiles[i].name;
			lookup.m_nPosition = newfiles[i].filepos + offsetofpackinmetafile;
			lookup.m_nLength   = newfiles[i].filelen;

			packfile.m_PackFiles.Insert(lookup);
		}

		packfile.m_nNumPackFiles = numpackfiles;
		delete [] newfiles;
		return true;
	}

	return false;
}

bool CBaseFileSystem::Prepare64BitPackFile(CSearchPath &packfile, int64_t offsetofpackinmetafile)
{
	packheader64_t header;
	FS_fseek(packfile.m_hPackFile->m_pFile, packfile.m_hPackFile->m_nStartOffset + offsetofpackinmetafile, FILESYSTEM_SEEK_HEAD);
	FS_fread(&header, 1, sizeof(header), packfile.m_hPackFile->m_pFile);

	if (*(uint32_t *)header.id != MAKEID('P', 'K', '6', '4'))
	{
		Warning(FILESYSTEM_WARNING, "%s is not a packfile\n", packfile.GetPath().String());
		return false;
	}

	int numpackfiles = header.dirlen / sizeof(packfile64_t);
	if (numpackfiles > MAX_DIR_PACK_FILES) {
		Warning(FILESYSTEM_WARNING, "%s has %i files\n", packfile.GetPath().String(), numpackfiles);
		return false;
	}

	if (numpackfiles > 0)
	{
		packfile64_t *newfiles = new packfile64_t[numpackfiles];
		if (!newfiles) {
			Warning(FILESYSTEM_WARNING, "%s out of memory allocating directory for %i files\n", packfile.GetPath().String(), numpackfiles);
			return false;
		}

		FS_fseek(packfile.m_hPackFile->m_pFile, packfile.m_hPackFile->m_nStartOffset + header.dirofs + offsetofpackinmetafile, FILESYSTEM_SEEK_HEAD);
		FS_fread(newfiles, 1, header.dirlen, packfile.m_hPackFile->m_pFile);

		for (int i = 0; i < numpackfiles; i++)
		{
			Q_strlwr(newfiles[i].name);
			FixSlashes(newfiles[i].name);

			CPackFileEntry lookup;
			lookup.m_Name      = newfiles[i].name;
			lookup.m_nPosition = newfiles[i].filepos + offsetofpackinmetafile;
			lookup.m_nLength   = newfiles[i].filelen;

			packfile.m_PackFiles.Insert(lookup);
		}

		packfile.m_nNumPackFiles = numpackfiles;
		delete [] newfiles;
		return true;
	}

	return false;
}

// Search pPath for pak?.pak files and add to search path if found
void CBaseFileSystem::AddPackFiles(const char *pPath)
{
	int pakcount;
	// determine pak files, [pak0..pakN]
	for (pakcount = 0; ; pakcount++)
	{
		char pakfile[MAX_PATH], fullpath[MAX_PATH];
		Q_sprintf(pakfile, "pak%i.pak", pakcount);
		Q_sprintf(fullpath, "%s%s", pPath, pakfile);
		FixSlashes(fullpath);

		struct _stat buf;
		if (FS_stat(fullpath, &buf) == -1)
			break;
	}

	// Add any pack files in the format pak1.pak ... pak0.pak
	// Add them backwards so pak(N) is higher priority than pak(N-1), etc.
	for (int i = pakcount - 1; i >= 0; i--)
	{
		char pakfile[512];
		Q_sprintf(pakfile, "pak%i.pak", i);
		AddPackFileFromPath(pPath, pakfile, false, "");
	}
}

// Wipe all map (.bsp) pak file search paths
void CBaseFileSystem::RemoveAllMapSearchPaths()
{
	int c = m_SearchPaths.Count();
	for (int i = c - 1; i >= 0; i--)
	{
		if (!m_SearchPaths[i].IsMapPath())
			continue;

		m_SearchPaths.Remove(i);
	}
}

// Performs a simple case-insensitive string comparison, honoring trailing * wildcards
bool IsWildCardMatch(const char *wildcardString, const char *stringToCheck)
{
	char wcChar;
	char strChar;

	if (!Q_strcmp(stringToCheck, ".") || !Q_strcmp(stringToCheck, ".."))
	{
		// ignore containing the levels directory
		return false;
	}

	if (!Q_strcmp(wildcardString, "*.*") || !Q_strcmp(wildcardString, "*"))
	{
		// matches everything
		return true;
	}

	// use the starMatchesZero variable to determine whether an asterisk
	// matches zero or more characters (TRUE) or one or more characters (FALSE)
	bool starMatchesZero = true;

	while ((strChar = *stringToCheck) && (wcChar = *wildcardString))
	{
		// we only want to advance the pointers if we successfully assigned
		// both of our char variables, so we'll do it here rather than in the
		// loop condition itself
		stringToCheck++;
		wildcardString++;

		// if this isn't a case-sensitive match, make both chars uppercase
		wcChar = toupper(wcChar);
		strChar = toupper(strChar);

		// check the wcChar against our wildcard list
		switch (wcChar)
		{
		// an asterisk matches zero or more characters
		case '*' :
			// do a recursive call against the rest of the string,
			// until we've either found a match or the string has
			// ended
			if (starMatchesZero)
				stringToCheck--;

			while (*stringToCheck)
			{
				if (IsWildCardMatch(wildcardString, stringToCheck++))
					return true;
			}

			break;

		// a question mark matches any single character
		case '?' :
			break;

		// if we fell through, we want an exact match
		default :
			if (wcChar != strChar)
				return false;
			break;
		}
	}

	// if we have any asterisks left at the end of the wildcard string, we can
	// advance past them if starMatchesZero is TRUE (so "blah*" will match "blah")
	while (*wildcardString && starMatchesZero)
	{
		if (*wildcardString == '*')
			wildcardString++;
		else
			break;
	}

	// if we got to the end but there's still stuff left in either of our strings,
	// return false; otherwise, we have a match
	if (*stringToCheck || *wildcardString)
		return false;
	else
		return true;
}

const char *CBaseFileSystem::SearchPakFile(const char *pWildCard, int currentSearchPathID, bool first)
{
	auto &curSearch = m_SearchPaths[currentSearchPathID];

	if (first) {
		curSearch.m_iCurSearchFile = 0;
	}

	while (curSearch.m_iCurSearchFile < curSearch.m_PackFiles.Count())
	{
		const char *file = curSearch.m_PackFiles[curSearch.m_iCurSearchFile++].m_Name.String();
		if (IsWildCardMatch(pWildCard, file)) {
			return file;
		}
	}

	return nullptr;
}

const char *CBaseFileSystem::FindFirst(const char *pWildCard, FileFindHandle_t *pHandle, const char *pathID)
{
 	Assert(pWildCard);
 	Assert(pHandle);

	FileFindHandle_t hTmpHandle = m_FindData.AddToTail();
	FindData_t *pFindData = &m_FindData[hTmpHandle];
	Assert(pFindData);

	int maxlen = Q_strlen(pWildCard) + 1;
	pFindData->m_WildCardString.AddMultipleToTail(maxlen);
	Q_strcpy(pFindData->m_WildCardString.Base(), pWildCard);
	FixSlashes(pFindData->m_WildCardString.Base());

	CUtlSymbol lookup(pathID);
	int c = m_SearchPaths.Count();
	for (pFindData->m_CurrentSearchPathID = 0; pFindData->m_CurrentSearchPathID < c; pFindData->m_CurrentSearchPathID++)
	{
		if (pathID && m_SearchPaths[pFindData->m_CurrentSearchPathID].GetPathID() != lookup) {
			continue;
		}

		pFindData->m_LimitedPathID = pathID ? (UtlSymId_t)lookup : -1;

		const char *fileName = FindFirstHelper(pWildCard, pHandle, pFindData->m_CurrentSearchPathID, pFindData);
		if (fileName) {
			return fileName;
		}
	}

	// Handle failure here
	pFindData = nullptr;
	m_FindData.Remove(hTmpHandle);
	return nullptr;
}

const char *CBaseFileSystem::FindFirstHelper(const char *pWildCard, FileFindHandle_t *pHandle, int searchPath, FindData_t *pFindData)
{
	CSearchPath *pSearchPath = &m_SearchPaths[searchPath];
	if (pSearchPath->IsPackFile())
	{
		const char *fname = SearchPakFile(pFindData->m_WildCardString.Base(), searchPath);
		if (fname) {
			pFindData->m_FindHandle = INVALID_HANDLE_VALUE;
			*pHandle = m_FindData.Count() - 1;
			return fname;
		}
	}
	else
	{
		int len = Q_strlen(pSearchPath->GetPath().String()) + pFindData->m_WildCardString.Size();
		char *pTmpFileName = (char *)alloca((len + 1) * sizeof(char));
		Q_strcpy(pTmpFileName, pSearchPath->GetPath().String());
		Q_strcat(pTmpFileName, pFindData->m_WildCardString.Base());
		FixSlashes(pTmpFileName);

		if ((pFindData->m_FindHandle = FS_FindFirstFile(pTmpFileName, &pFindData->m_FindData)) != INVALID_HANDLE_VALUE)
		{
			*pHandle = m_FindData.Count() - 1;
			return pFindData->m_FindData.cFileName;
		}
	}

	return nullptr;
}

bool CBaseFileSystem::FileInSearchPaths(const char *pSearchWildcard, const char *pFileName, int minSearchPathID, int maxSearchPathID)
{
	if (minSearchPathID > maxSearchPathID)
		return false;

	const char *tmp = &pSearchWildcard[Q_strlen(pSearchWildcard) - 1];
	for (; *tmp != CORRECT_PATH_SEPARATOR && pSearchWildcard < tmp; tmp--) {}

	if (++tmp <= pSearchWildcard)
		return false;

	int pathStrLen = tmp - pSearchWildcard;
	int fileNameStrLen = Q_strlen(pFileName);
	char *pFileNameWithPath = (char *)alloca((pathStrLen + fileNameStrLen + 1) * sizeof(char));
	Q_strncpy(pFileNameWithPath, pSearchWildcard, pathStrLen);
	pFileNameWithPath[pathStrLen] = '\0';
	Q_strcat(pFileNameWithPath, pFileName);

	for (int i = minSearchPathID; i <= maxSearchPathID; i++)
	{
		CSearchPath *pSearchPath = &m_SearchPaths[i];

		if (!pSearchPath->IsPackFile())
		{
			int len = Q_strlen(pFileNameWithPath) + Q_strlen(pSearchPath->GetPath().String());
			char *fullFilePath = (char *)alloca((len + 1) * sizeof(char));
			Q_strcpy(fullFilePath, pSearchPath->GetPath().String());
			Q_strcat(fullFilePath, pFileNameWithPath);

			struct _stat buf;
			if (FS_stat(fullFilePath, &buf) != -1) {
				return true;
			}
		}
		else
		{
			int curSearch = pSearchPath->m_iCurSearchFile;
			bool ret = SearchPakFile(pSearchWildcard, i) ? true : false;
			pSearchPath->m_iCurSearchFile = curSearch;

			if (ret) {
				return ret;
			}
		}
	}

	return false;
}

// Get the next file, trucking through the path. Don't check for duplicates.
bool CBaseFileSystem::FindNextFileHelper(FindData_t *pFindData)
{
	if (m_SearchPaths[pFindData->m_CurrentSearchPathID].IsPackFile())
	{
		const char *file = SearchPakFile(pFindData->m_WildCardString.Base(), pFindData->m_CurrentSearchPathID, false);
		if (file)
		{
			char *pFileNameNoPath = const_cast<char *>(Q_strrchr(file, CORRECT_PATH_SEPARATOR));
			if (pFileNameNoPath)
			{
				file = pFileNameNoPath + 1;
			}

			Q_strlcpy(pFindData->m_FindData.cFileName, file);
			FixSlashes(pFindData->m_FindData.cFileName);
			return true;
		}
	}
	// Try the same search path that we were already searching on
	else if (FS_FindNextFile(pFindData->m_FindHandle, &pFindData->m_FindData))
	{
		return true;
	}

	// This happens when we searched a full path
	// if (pFindData->m_CurrentSearchPathID < 0)
	//	return false;

	pFindData->m_CurrentSearchPathID++;

	if (pFindData->m_FindHandle != INVALID_HANDLE_VALUE)
	{
		FS_FindClose(pFindData->m_FindHandle);
	}

	pFindData->m_FindHandle = INVALID_HANDLE_VALUE;

	int c = m_SearchPaths.Count();
	for(; pFindData->m_CurrentSearchPathID < c; pFindData->m_CurrentSearchPathID++)
	{
		CSearchPath *pSearchPath = &m_SearchPaths[pFindData->m_CurrentSearchPathID];

		if (pFindData->m_LimitedPathID != -1)
		{
			if (pSearchPath->GetPath() != pFindData->m_LimitedPathID) {
				return false;
			}
		}

		if (pSearchPath->IsPackFile())
		{
			const char *file = SearchPakFile(pFindData->m_WildCardString.Base(), pFindData->m_CurrentSearchPathID);
			if (file)
			{
				Q_strlcpy(pFindData->m_FindData.cFileName, Q_strrchr(file, CORRECT_PATH_SEPARATOR) + 1);
				FixSlashes(pFindData->m_FindData.cFileName);
				return true;
			}
		}
		else
		{
			int len = Q_strlen(pSearchPath->GetPath().String()) + pFindData->m_WildCardString.Size();
			char *pTmpFileName = (char *)alloca((len + 1) * sizeof(char));
			Q_strcpy(pTmpFileName, pSearchPath->GetPath().String());
			Q_strcat(pTmpFileName, pFindData->m_WildCardString.Base());
			FixSlashes(pTmpFileName);

			if ((pFindData->m_FindHandle = FS_FindFirstFile(pTmpFileName, &pFindData->m_FindData)) != INVALID_HANDLE_VALUE)
			{
				return true;
			}
		}
	}

	return false;
}

const char *CBaseFileSystem::FindNext(FileFindHandle_t handle)
{
	if (!m_FindData.Count() || handle >= m_FindData.Count()) {
		return nullptr;
	}

	FindData_t *pFindData = &m_FindData[handle];
	if (FindNextFileHelper(pFindData))
	{
		return pFindData->m_FindData.cFileName;
	}

	return nullptr;
}

bool CBaseFileSystem::FindIsDirectory(FileFindHandle_t handle)
{
	FindData_t *pFindData = &m_FindData[handle];
	return (pFindData->m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
}

void CBaseFileSystem::FindClose(FileFindHandle_t handle)
{
	if (!m_FindData.Count() || handle >= m_FindData.Count())
		return;

	FindData_t *pFindData = &m_FindData[handle];
	Assert(pFindData);

	if (pFindData->m_FindHandle != INVALID_HANDLE_VALUE)
	{
		FS_FindClose(pFindData->m_FindHandle);
	}

	pFindData->m_FindHandle = INVALID_HANDLE_VALUE;

	pFindData->m_WildCardString.Purge();
	m_FindData.FastRemove(handle);
}

char *CBaseFileSystem::ParseFile(char *pFileBytes, char *pToken, bool *pWasQuoted)
{
	return ::ParseFile(pFileBytes, pToken, pWasQuoted);
}

long CBaseFileSystem::GetFileTime(const char *pFileName)
{
	for (int i = 0; i < m_SearchPaths.Count(); i++)
	{
		FileHandle_t filehandle = FindFile(&m_SearchPaths[i], pFileName, "rb");
		if (filehandle)
		{
			CFileHandle *fh = reinterpret_cast<CFileHandle *>(filehandle);
			long time = fh->m_nFileTime;
			Close(filehandle);
			return time;
		}
	}

	return 0L;
}

void CBaseFileSystem::FileTimeToString(char *pString, int maxCharsIncludingTerminator, long fileTime)
{
	time_t time = fileTime;
	Q_strncpy(pString, ctime(&time), maxCharsIncludingTerminator);
	pString[maxCharsIncludingTerminator - 1] = '\0';
}

void CBaseFileSystem::SetWarningFunc(WarningFunc_t pfnWarning)
{
	m_pfnWarning = pfnWarning;
}

void CBaseFileSystem::SetWarningLevel(FileWarningLevel_t level)
{
	m_fwLevel = level;
}

void CBaseFileSystem::Warning(FileWarningLevel_t level, const char *fmt, ...)
{
	if (m_fwLevel < level) {
		return;
	}

	va_list argptr;
	char warningtext[4096];

	va_start(argptr, fmt);
	Q_vsnprintf(warningtext, sizeof(warningtext), fmt, argptr);
	va_end(argptr);

	if (m_pfnWarning)
	{
		m_pfnWarning(warningtext);
	}
	else
	{
#ifdef _WIN32
		OutputDebugString(warningtext);
#else
		fprintf(stderr, "%s", warningtext);
#endif
	}
}

bool CBaseFileSystem::FullPathToRelativePath(const char *pFullpath, char *pRelative)
{
	int inlen = Q_strlen(pFullpath);
	if (inlen <= 0) {
		*pRelative = '\0';
		return false;
	}

	Q_strcpy(pRelative, pFullpath);

	char *inpath = (char *)alloca((inlen + 1) * sizeof(char));
	Q_strcpy(inpath, pFullpath);

#ifdef _WIN32 // don't do this on linux!
	Q_strlwr(inpath);
#endif

	FixSlashes(inpath);

	bool success = false;
	for (int i = 0; i < m_SearchPaths.Count() && !success; i++)
	{
		CSearchPath *pSearchPath = &m_SearchPaths[i];
		if (pSearchPath->IsMapPath())
			continue;

		char *searchbase = new char [Q_strlen(pSearchPath->GetPath().String()) + 1];
		Q_strcpy(searchbase, pSearchPath->GetPath().String());
		FixSlashes(searchbase);

#ifdef _WIN32 // don't do this on linux!
		Q_strlwr(searchbase);
#endif

		int baselen = Q_strlen(searchbase);
		if (!Q_strnicmp(searchbase, inpath, baselen))
		{
			success = true;
			Q_strcpy(pRelative, &inpath[baselen]);
		}

		delete [] searchbase;
	}

	return success;
}

bool CBaseFileSystem::OpenedFileLessFunc(COpenedFile const &src1, COpenedFile const &src2)
{
	return src1.m_pFile < src2.m_pFile;
}

void CBaseFileSystem::RemoveAllSearchPaths()
{
	m_SearchPaths.Purge();
	m_PackFileHandles.Purge();
}

void CBaseFileSystem::StripFilename(char *path)
{
	int length = Q_strlen(path);
	if (length <= 0)
		return;

	length--;
	while (length > 0)
	{
		if (path[length] == CORRECT_PATH_SEPARATOR
			|| path[length] == INCORRECT_PATH_SEPARATOR)
			break;

		length--;
	}

	path[length] = '\0';
}

void CBaseFileSystem::FixSlashes(char *str)
{
	auto ptr = str;
	while (*str)
	{
		if (*str == INCORRECT_PATH_SEPARATOR) {
			*str = CORRECT_PATH_SEPARATOR;
		}

		str++;
	}
}

void CBaseFileSystem::FixPath(char *str)
{
	if (!str[0])
		return;

	char *lastChar = &str[Q_strlen(str) - 1];
	if (*lastChar != CORRECT_PATH_SEPARATOR && *lastChar != INCORRECT_PATH_SEPARATOR)
	{
		lastChar[1] = CORRECT_PATH_SEPARATOR;
		lastChar[2] = '\0';
	}

	FixSlashes(str);
}

CBaseFileSystem::COpenedFile::COpenedFile() : m_pFile(nullptr), m_pName(nullptr)
{
}

CBaseFileSystem::COpenedFile::~COpenedFile()
{
	if (m_pName) {
		delete[] m_pName;
		m_pName = nullptr;
	}
}

CBaseFileSystem::COpenedFile::COpenedFile(COpenedFile const &src)
{
	m_pFile = src.m_pFile;
	m_pName = nullptr;

	if (src.m_pName)
	{
		m_pName = new char [Q_strlen(src.m_pName) + 1];
		Q_strcpy(m_pName, src.m_pName);
	}
}

bool CBaseFileSystem::COpenedFile::operator==(COpenedFile const &src) const
{
	return m_pFile == src.m_pFile;
}

void CBaseFileSystem::COpenedFile::SetName(const char *name)
{
	if (m_pName) {
		delete[] m_pName;
	}

	m_pName = new char [Q_strlen(name) + 1];
	Q_strcpy(m_pName, name);
}

const char *CBaseFileSystem::COpenedFile::GetName()
{
	return m_pName ? m_pName : "???";
}

bool CBaseFileSystem::CSearchPath::PackFileLessFunc(CPackFileEntry const &src1, CPackFileEntry const &src2)
{
	return src1.m_Name < src2.m_Name;
}

CBaseFileSystem::CSearchPath::CSearchPath() :
	m_Path(CUtlSymbol("")),
	m_bIsMapPath(false),
	m_bIsPackFile(false),
	m_lPackFileTime(0),
	m_hPackFile(nullptr),
	m_nNumPackFiles(0),
	m_iCurSearchFile(0),
	m_bAllowWrite(true),
	m_PackFiles(0, MAX_ENTRY_PATH, CSearchPath::PackFileLessFunc)
{
}

CBaseFileSystem::CSearchPath::~CSearchPath()
{
	if (m_bIsPackFile && m_hPackFile)
	{
		BaseFileSystem()->m_PackFileHandles.FindAndRemove(m_hPackFile->m_pFile);
		BaseFileSystem()->Close(m_hPackFile->m_pFile);
	}
}
