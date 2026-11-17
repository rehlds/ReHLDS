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

#include "interface.h"
#include <stdio.h>
#include <stdlib.h>

// There is only one instance of the IFileSystem interface,
// located in the filesystem_stdio library (filesystem_steam is obsolete).
#ifdef _WIN32
	#define STDIO_FILESYSTEM_LIB "filesystem_stdio.dll"
	#define STEAM_FILESYSTEM_LIB "filesystem_steam.dll"
#else
	#define STDIO_FILESYSTEM_LIB "filesystem_stdio.so"
	#define STEAM_FILESYSTEM_LIB "filesystem_steam.so"
#endif // _WIN32

// Forward declarations
typedef FILE *FileHandle_t;
typedef int FileFindHandle_t;
typedef int WaitForResourcesHandle_t;
typedef void (*WarningFunc_t)(const char *fmt, ...);

// Enums used by the interface
#ifndef FILESYSTEM_INTERNAL_H
typedef enum
{
	FILESYSTEM_SEEK_HEAD = 0,
	FILESYSTEM_SEEK_CURRENT,
	FILESYSTEM_SEEK_TAIL,
} FileSystemSeek_t;

enum
{
	FILESYSTEM_INVALID_FIND_HANDLE = -1
};

typedef enum
{
	FILESYSTEM_WARNING = -1,					// A problem!
	FILESYSTEM_WARNING_QUIET = 0,				// Don't print anything
	FILESYSTEM_WARNING_REPORTUNCLOSED,			// On shutdown, report names of files left unclosed
	FILESYSTEM_WARNING_REPORTUSAGE,				// Report number of times a file was opened, closed
	FILESYSTEM_WARNING_REPORTALLACCESSES		// Report all open/close events to console (!slow!)
} FileWarningLevel_t;

const FileHandle_t FILESYSTEM_INVALID_HANDLE = nullptr;

#endif // FILESYSTEM_INTERNAL_H

// turn off any windows defines
#undef GetCurrentDirectory

// Purpose: Main file system interface
class IFileSystem: public IBaseInterface
{
public:
	// Mount and unmount the filesystem
	virtual void			Mount() = 0;
	virtual void			Unmount() = 0;

	// Remove all search paths (including write path?)
	virtual void			RemoveAllSearchPaths() = 0;

	// Add paths in priority order (mod dir, game dir, ....)
	// If one or more .pak files are in the specified directory, then they are
	//  added after the file system path
	// If the path is the relative path to a .bsp file, then any previous .bsp file
	//  override is cleared and the current .bsp is searched for an embedded PAK file
	//  and this file becomes the highest priority search path (i.e., it's looked at first
	//   even before the mod's file system path).
	virtual void			AddSearchPath(const char *pPath, const char *pathID) = 0;
	virtual bool			RemoveSearchPath(const char *pPath) = 0;

	// Deletes a file
	virtual void			RemoveFile(const char *pRelativePath, const char *pathID) = 0;

	// this isn't implementable on STEAM as is.
	virtual void			CreateDirHierarchy(const char *path, const char *pathID) = 0;

	// File I/O and info
	virtual bool			FileExists(const char *pFileName) = 0;
	virtual bool			IsDirectory(const char *pFileName) = 0;

	// opens a file
	// if pathID is NULL, all paths will be searched for the file
	virtual FileHandle_t	Open(const char *pFileName, const char *pOptions, const char *pathID = 0L) = 0;

	virtual void			Close(FileHandle_t file) = 0;

	virtual void			Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType) = 0;
	virtual size_t			Tell(FileHandle_t file) = 0;

	virtual size_t			Size(FileHandle_t file) = 0;
	virtual size_t			Size(const char *pFileName) = 0;

	virtual long			GetFileTime(const char *pFileName) = 0;
	virtual void			FileTimeToString(char *pStrip, int maxCharsIncludingTerminator, long fileTime) = 0;

	virtual bool			IsOk(FileHandle_t file) = 0;

	virtual void			Flush(FileHandle_t file) = 0;
	virtual bool			EndOfFile(FileHandle_t file) = 0;

	virtual int				Read(void *pOutput, int size, FileHandle_t file) = 0;
	virtual int				Write(void const *pInput, int size, FileHandle_t file) = 0;
	virtual char			*ReadLine(char *pOutput, int maxChars, FileHandle_t file) = 0;
	virtual int				FPrintf(FileHandle_t file, const char *fmt, ...) = 0;

	// direct filesystem buffer access
	// returns a handle to a buffer containing the file data
	// this is the optimal way to access the complete data for a file,
	// since the file preloader has probably already got it in memory
	virtual void			*GetReadBuffer(FileHandle_t file, int *outBufferSize, bool failIfNotInCache) = 0;
	virtual void			ReleaseReadBuffer(FileHandle_t file, void *readBuffer) = 0;

	// FindFirst/FindNext
	virtual const char		*FindFirst(const char *pWildCard, FileFindHandle_t *pHandle, const char *pathID = 0L) = 0;
	virtual const char		*FindNext(FileFindHandle_t handle) = 0;
	virtual bool			FindIsDirectory(FileFindHandle_t handle) = 0;
	virtual void			FindClose(FileFindHandle_t handle) = 0;

	virtual void			GetLocalCopy(const char *pFileName) = 0;

	virtual const char		*GetLocalPath(const char *pFileName, char *pLocalPath, int localPathBufferSize) = 0;

	// Note: This is sort of a secondary feature; but it's really useful to have it here
	virtual char			*ParseFile(char *pFileBytes, char *pToken, bool *pWasQuoted) = 0;

	// Returns true on success (based on current list of search paths, otherwise false if it can't be resolved)
	virtual bool			FullPathToRelativePath(const char *pFullpath, char *pRelative) = 0;

	// Gets the current working directory
	virtual bool			GetCurrentDirectory(char *pDirectory, int maxlen) = 0;

	// Dump to printf/OutputDebugString the list of files that have not been closed
	virtual void			PrintOpenedFiles() = 0;

	virtual void			SetWarningFunc(WarningFunc_t pfnWarning) = 0;
	virtual void			SetWarningLevel(FileWarningLevel_t level) = 0;

	virtual void			LogLevelLoadStarted(const char *name) = 0;
	virtual void			LogLevelLoadFinished(const char *name) = 0;
	virtual int				HintResourceNeed(const char *hintlist, int forgetEverything) = 0;
	virtual int				PauseResourcePreloading() = 0;
	virtual int				ResumeResourcePreloading() = 0;
	virtual int				SetVBuf(FileHandle_t stream, char *buffer, int mode, long size) = 0;
	virtual void			GetInterfaceVersion(char *p, int maxlen) = 0;
	virtual bool			IsFileImmediatelyAvailable(const char *pFileName) = 0;

	// starts waiting for resources to be available
	// returns FILESYSTEM_INVALID_HANDLE if there is nothing to wait on
	virtual WaitForResourcesHandle_t WaitForResources(const char *resourcelist) = 0;

	// get progress on waiting for resources; progress is a float [0, 1], complete is true on the waiting being done
	// returns false if no progress is available
	// any calls after complete is true or on an invalid handle will return false, 0.0f, true
	virtual bool			GetWaitForResourcesProgress(WaitForResourcesHandle_t handle, float *progress /* out */ , bool *complete /* out */) = 0;

	// cancels a progress call
	virtual void			CancelWaitForResources(WaitForResourcesHandle_t handle) = 0;
	// returns true if the appID has all its caches fully preloaded
	virtual bool			IsAppReadyForOfflinePlay(int appID) = 0;

	// interface for custom pack files > 4Gb
	virtual bool			AddPackFile(const char *fullpath, const char *pathID) = 0;

	// open a file but force the data to come from the steam cache, NOT from disk
	virtual FileHandle_t	OpenFromCacheForRead(const char *pFileName, const char *pOptions, const char *pathID = 0L) = 0;
	virtual void			AddSearchPathNoWrite(const char *pPath, const char *pathID) = 0;

	// Added in the Half-Life 25th Anniversary SDK (2024); same interface version
	virtual long			GetFileModificationTime(const char *pFileName) = 0;
};

// Steam3/Src compat
#define IBaseFileSystem IFileSystem

#define FILESYSTEM_INTERFACE_VERSION "VFileSystem009"
