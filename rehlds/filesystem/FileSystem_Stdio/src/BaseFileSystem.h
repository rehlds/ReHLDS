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

#pragma once

#include <FileSystem.h>
#include "filesystem_helpers.h"

#ifdef _WIN32
	#include <direct.h> // mkdir
#else
	#include <unistd.h> // unlink
	#include "linux_support.h"

	// undo the prepended "_" 's
	#define _chmod chmod
	#define _stat stat
	#define _alloca alloca
	#define _S_IFDIR S_IFDIR

	#define HANDLE int
	#define INVALID_HANDLE_VALUE ((HANDLE)~0)
#endif // _WIN32

#include "utlsymbol.h"

class CBaseFileSystem: public IFileSystem {
public:
	CBaseFileSystem();
	virtual ~CBaseFileSystem() {};

	// Mount and unmount the filesystem
	virtual void Mount();
	virtual void Unmount();

	// opens a file
	// if pathID is NULL, all paths will be searched for the file
	virtual FileHandle_t Open(const char *pFileName, const char *pOptions, const char *pathID = 0L);

	// open a file but force the data to come from the steam cache, NOT from disk
	virtual FileHandle_t OpenFromCacheForRead(const char *pFileName, const char *pOptions, const char *pathID = 0L);

	virtual void Close(FileHandle_t file);

	virtual void Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType);
	virtual size_t Tell(FileHandle_t file);

	virtual size_t Size(FileHandle_t file);
	virtual size_t Size(const char *pFileName);

	virtual bool IsOk(FileHandle_t file);

	virtual void Flush(FileHandle_t file);
	virtual bool EndOfFile(FileHandle_t file);

	virtual int Read(void* pOutput, int size, FileHandle_t file);
	virtual int Write(void const* pInput, int size, FileHandle_t file);
	virtual char *ReadLine(char *pOutput, int maxChars, FileHandle_t file);
	virtual int FPrintf(FileHandle_t file, const char *fmt, ...);

	// direct filesystem buffer access
	// returns a handle to a buffer containing the file data
	// this is the optimal way to access the complete data for a file,
	// since the file preloader has probably already got it in memory
	virtual void *GetReadBuffer(FileHandle_t file, int *outBufferSize, bool failIfNotInCache);
	virtual void ReleaseReadBuffer(FileHandle_t file, void *readBuffer);

	// Gets the current working directory
	virtual bool GetCurrentDirectory(char* pDirectory, int maxlen);

	// this isn't implementable on STEAM as is.
	virtual void CreateDirHierarchy(const char *path, const char *pathID);

	// File I/O and info
	virtual bool IsDirectory(const char *pFileName);
	virtual const char *GetLocalPath(const char *pFileName, char *pLocalPath, int localPathBufferSize);

	// Deletes a file
	virtual void RemoveFile(const char *pRelativePath, const char *pathID);

	// Remove all search paths (including write path?)
	virtual void RemoveAllSearchPaths();

	// Add paths in priority order (mod dir, game dir, ....)
	// If one or more .pak files are in the specified directory, then they are
	// added after the file system path
	// If the path is the relative path to a .bsp file, then any previous .bsp file
	// override is cleared and the current .bsp is searched for an embedded PAK file
	// and this file becomes the highest priority search path (i.e., it's looked at first
	// even before the mod's file system path).
	virtual void AddSearchPath(const char *pPath, const char *pathID);
	virtual bool RemoveSearchPath(const char *pPath);

	virtual bool FileExists(const char *pFileName);

	virtual long GetFileTime(const char *pFileName);
	virtual void FileTimeToString(char* pStrip, int maxCharsIncludingTerminator, long fileTime);

	// FindFirst/FindNext
	virtual const char *FindFirst(const char *pWildCard, FileFindHandle_t *pHandle, const char *pathID = 0L);
	virtual const char *FindNext(FileFindHandle_t handle);
	virtual bool FindIsDirectory(FileFindHandle_t handle);
	virtual void FindClose(FileFindHandle_t handle);

	// Note: This is sort of a secondary feature; but it's really useful to have it here
	virtual char *ParseFile(char* pFileBytes, char* pToken, bool* pWasQuoted);

	// Returns true on success (based on current list of search paths, otherwise false if it can't be resolved)
	virtual bool FullPathToRelativePath(const char *pFullpath, char *pRelative);

	// Dump to printf/OutputDebugString the list of files that have not been closed
	virtual void PrintOpenedFiles();

	virtual void SetWarningFunc(WarningFunc_t pfnWarning);
	virtual void SetWarningLevel(FileWarningLevel_t level);

	// interface for custom pack files > 4Gb
	virtual bool AddPackFile(const char *fullpath, const char *pathID);

	FILE *Trace_FOpen(const char *filename, const char *options, bool bFromCache = false);
	void Trace_FClose(FILE *fp);
	void Trace_DumpUnclosedFiles();

	// Purpose: Functions implementing basic file system behavior.
	virtual FILE *FS_fopen(const char *filename, const char *options, bool bFromCache = false) = 0;
	virtual void FS_fclose(FILE *fp) = 0;
	virtual void FS_fseek(FILE *fp, int64_t pos, FileSystemSeek_t seekType) = 0;
	virtual size_t FS_ftell(FILE *fp) = 0;
	virtual int FS_feof(FILE *fp) = 0;
	virtual size_t FS_fread(void *dest, size_t count, size_t size, FILE *fp) = 0;
	virtual size_t FS_fwrite(const void *src, size_t count, size_t size, FILE *fp) = 0;
	virtual size_t FS_vfprintf(FILE *fp, const char *fmt, va_list list) = 0;
	virtual int FS_ferror(FILE *fp) = 0;
	virtual int FS_fflush(FILE *fp) = 0;
	virtual char *FS_fgets(char *dest, int destSize, FILE *fp) = 0;
	virtual int FS_stat(const char *path, struct _stat *buf) = 0;
	virtual HANDLE FS_FindFirstFile(char *findname, WIN32_FIND_DATA *dat) = 0;
	virtual bool FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA *dat) = 0;
	virtual bool FS_FindClose(HANDLE handle) = 0;
	virtual bool IsThreadSafe() = 0;
	virtual bool IsFileImmediatelyAvailable(const char *pFileName) = 0;

protected:
	// structures for building pack files
	#pragma pack(1)
	typedef struct
	{
		char        name[56];
		int         filepos;
		int         filelen;
	} packfile_t;

	typedef struct
	{
		char id[4];
		int dirofs;
		int dirlen;
	} packheader_t;

	typedef struct
	{
		char        name[112];
		int64_t     filepos;
		int64_t     filelen;
	} packfile64_t;

	typedef struct
	{
		char        id[4];
		int64_t     dirofs;
		int64_t     dirlen;
	} packheader64_t;

	typedef struct
	{
		char        id[8];
		int64_t     packheaderpos;
		int64_t     originalfilesize;
	} packappenededheader_t;
	#pragma pack()

	// A Pack file directory entry:
	class CPackFileEntry
	{
	public:
		CUtlSymbol m_Name;
		int64_t    m_nPosition;
		int64_t    m_nLength;
	};

	class CFileHandle
	{
	public:
		FILE*       m_pFile;
		bool        m_bPack;
		bool        m_bErrorFlagged;
		int64_t     m_nStartOffset;
		int64_t     m_nLength;
		long        m_nFileTime;
	};

	enum { MAX_FILES_IN_PACK = 32768 };
	class CSearchPath
	{
	public:
		CSearchPath();
		~CSearchPath();

		// Path ID ("game", "mod", "gamebin") accessors.
		void SetPathID(CUtlSymbol id);
		const CUtlSymbol &GetPathID() const;

		// Search path (c:\hl\hl) accessors.
		void SetPath(CUtlSymbol id);
		const CUtlSymbol &GetPath() const;

		void SetWritePath(bool st);

		bool IsAllowWrite() const;
		bool IsPackFile() const;
		bool IsMapPath() const;

	private:
		friend class CBaseFileSystem;

		CUtlSymbol      m_Path;
		CUtlSymbol      m_PathID;

		bool            m_bIsMapPath;
		bool            m_bIsPackFile;

		long            m_lPackFileTime;
		CFileHandle*    m_hPackFile;
		int             m_nNumPackFiles;
		size_t          m_iCurSearchFile;
		bool            m_bAllowWrite;

		enum { MAX_ENTRY_PATH = 32 };

		// Entries to the individual files stored inside the pack file.
		CUtlRBTree<CPackFileEntry, int> m_PackFiles;
		static bool PackFileLessFunc(CPackFileEntry const &src1, CPackFileEntry const &src2);
	};

	// Purpose: For tracking unclosed files
	// NOTE: The symbol table could take up memory that we don't want to eat here.
	// In that case, we shouldn't store them in a table, or we should store them as locally allocates stings
	// so we can control the size
	class COpenedFile
	{
	public:
		COpenedFile();
		~COpenedFile();

		COpenedFile(COpenedFile const &src);
		bool operator==(COpenedFile const &src) const;

		void SetName(char const *name);
		const char *GetName();

		FILE *m_pFile;
		char *m_pName;
	};

	struct FindData_t
	{
		WIN32_FIND_DATA  m_FindData;
		int              m_CurrentSearchPathID;
		int              m_LimitedPathID;
		CUtlVector<char> m_WildCardString;
		HANDLE           m_FindHandle;
	};

	virtual void AddSearchPathNoWrite(const char *pPath, const char *pathID);
	virtual long GetFileModificationTime(const char *pFileName);
	void AddSearchPathInternal(const char *pPath, const char *pathID, bool bAllowWrite);

	void Warning(FileWarningLevel_t level, const char *fmt, ...);
	void FixSlashes(char *str);
	void FixPath(char *str);
	void StripFilename(char *path);
	CSearchPath *GetWritePath(const char *pathID);
	FileHandle_t FindFile(CSearchPath *path, const char *pFileName, const char *pOptions, bool bFromCache = false);
	int FastFindFileSize(CSearchPath *path, const char *pFileName);
	void RemoveAllMapSearchPaths();
	void AddPackFiles(const char *pPath);
	bool AddPackFileFromPath(const char *pPath, const char *pakfile, bool bCheckForAppendedPack, const char *pathID);
	bool PreparePackFile(CSearchPath &packfile, int64_t offsetofpackinmetafile);
	bool Prepare64BitPackFile(CSearchPath &packfile, int64_t offsetofpackinmetafile);
	const char *SearchPakFile(const char *pWildCard, int currentSearchPathID, bool first = true);
	bool FileInSearchPaths(const char *pSearchWildcard, const char *pFileName, int minSearchPathID, int maxSearchPathID);
	bool FindNextFileHelper(FindData_t *pFindData);
	const char *FindFirstHelper(const char *pWildCard, FileFindHandle_t *pHandle, int searchPath, FindData_t *pFindData);

public:
	static CBaseFileSystem *s_pFileSystem;

protected:
	CUtlVector<COpenedFile>    m_OpenedFiles;
	static bool OpenedFileLessFunc(COpenedFile const &src1, COpenedFile const &src2);

	CUtlVector<FILE *>         m_PackFileHandles;
	CUtlVector<FindData_t>     m_FindData;
	CUtlVector<CSearchPath>    m_SearchPaths;

	FileWarningLevel_t         m_fwLevel;
	WarningFunc_t              m_pfnWarning;

	int                        m_nOpenCount;
};

// singleton accessor
CBaseFileSystem *BaseFileSystem();

// Inlines
inline void CBaseFileSystem::CSearchPath::SetPathID(CUtlSymbol sym)
{
	m_PathID = sym;
}

inline const CUtlSymbol &CBaseFileSystem::CSearchPath::GetPathID() const
{
	return m_PathID;
}

inline void CBaseFileSystem::CSearchPath::SetPath(CUtlSymbol id)
{
	m_Path = id;
}

inline const CUtlSymbol &CBaseFileSystem::CSearchPath::GetPath() const
{
	return m_Path;
}

inline void CBaseFileSystem::CSearchPath::SetWritePath(bool st)
{
	m_bAllowWrite = st;
}

inline bool CBaseFileSystem::CSearchPath::IsAllowWrite() const
{
	return m_bAllowWrite;
}

inline bool CBaseFileSystem::CSearchPath::IsPackFile() const
{
	return m_bIsPackFile;
}

inline bool CBaseFileSystem::CSearchPath::IsMapPath() const
{
	return m_bIsMapPath;
}
