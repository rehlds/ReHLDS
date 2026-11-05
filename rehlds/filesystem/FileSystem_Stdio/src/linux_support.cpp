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

#if !defined(_WIN32)

char selectBuf[PATH_MAX];
int FileSelect(const struct dirent *ent)
{
	const char *mask = selectBuf;
	const char *name = ent->d_name;

	//printf("Test:%s %s\n", mask, name);

	if (!Q_strcmp(name, ".") || !Q_strcmp(name, ".."))
		return 0;

	if (!Q_strcmp(selectBuf, "*.*"))
		return 1;

	while (*mask && *name)
	{
		if (*mask == '*')
		{
			mask++; // move to the next char in the mask
			if (!*mask) // if this is the end of the mask its a match
			{
				return 1;
			}
			while (*name && toupper(*name) != toupper(*mask))
			{
				// while the two don't meet up again
				name++;
			}
			if (!*name)
			{
				// end of the name
				break;
			}
		}
		else if (*mask != '?')
		{
			if (toupper(*mask) != toupper(*name))
			{
				// mismatched!
				return 0;
			}
			else
			{
				mask++;
				name++;
				if (!*mask && !*name)
				{
					// if its at the end of the buffer
					return 1;
				}

			}

		}
		// mask is "?", we don't care
		else
		{
			mask++;
			name++;
		}
	}

	// both of the strings are at the end
	return (!*mask && !*name);
}

int FillDataStruct(FIND_DATA *dat)
{
	char szFileName[MAX_PATH];
	struct stat fileStat;

	if (dat->numMatches < 0)
		return -1;

	Q_strlcpy(szFileName, dat->cDir);
	Q_strlcat(szFileName, "/");
	Q_strlcat(szFileName, dat->namelist[dat->numMatches]->d_name);
	Q_strlcpy(dat->cFileName, dat->namelist[dat->numMatches]->d_name);

	if (!stat(szFileName, &fileStat))
	{
		dat->dwFileAttributes = fileStat.st_mode;
	}
	else
	{
		dat->dwFileAttributes = 0;
	}

	// printf("%s\n", dat->namelist[dat->numMatches]->d_name);
	free(dat->namelist[dat->numMatches]);
	dat->numMatches--;
	return 1;
}

int FindFirstFile(char *fileName, FIND_DATA *dat)
{
	char nameStore[PATH_MAX];
	char *dir = nullptr;
	int n, iret = -1;

	Q_strlcpy(nameStore, fileName);

	if (Q_strrchr(nameStore, '/'))
	{
		dir = nameStore;
		while (Q_strrchr(dir, '/'))
		{
			struct stat dirChk;

			// zero this with the dir name
			dir = Q_strrchr(nameStore, '/');
			*dir = '\0';

			dir = nameStore;
			stat(dir, &dirChk);

			if (S_ISDIR(dirChk.st_mode))
			{
				break;
			}
		}
	}
	else
	{
		// couldn't find a dir seperator...
		return -1;
	}

	if (Q_strlen(dir) > 0)
	{
		Q_strlcpy(selectBuf, fileName + Q_strlen(dir) + 1);
		Q_strlcpy(dat->cDir, dir);

		n = scandir(dir, &dat->namelist, FileSelect, alphasort);
		if (n < 0)
		{
			// silently return, nothing interesting
			printf("scandir failed:%s\n", dir);
		}
		else
		{
			// n is the number of matches
			dat->numMatches = n - 1;
			iret = FillDataStruct(dat);
			if (iret < 0)
			{
				free(dat->namelist);
			}
		}
	}

	// printf("Returning: %i \n", iret);
	return iret;
}

bool FindNextFile(int handle, FIND_DATA *dat)
{
	if (dat->numMatches < 0)
	{
		free(dat->namelist);
		return false; // no matches left
	}

	FillDataStruct(dat);
	return true;
}

bool FindClose(int handle)
{
	return true;
}

static char fileName[MAX_PATH];
int CheckName(const struct dirent *dir)
{
	return !Q_stricmp(dir->d_name, fileName);
}

const char *findFileInDirCaseInsensitive(const char *file)
{
	const char *dirSep = Q_strrchr(file, '/');
	if (!dirSep)
	{
		dirSep = Q_strrchr(file, '\\');
		if (!dirSep)
		{
			return nullptr;
		}
	}

	char *dirName = static_cast<char *>(alloca(((dirSep - file) + 1) * sizeof(char)));
	if (!dirName) {
		return nullptr;
	}

	Q_strncpy(dirName, file, dirSep - file);
	dirName[dirSep - file] = '\0';

	struct dirent **namelist;
	int n;

	Q_strlcpy(fileName, dirSep + 1);
	n = scandir(dirName, &namelist, CheckName, alphasort);

	if (n > 0)
	{
		Q_snprintf(fileName, sizeof(fileName), "%s/%s", dirName, namelist[0]->d_name);

		// Free the malloc'd strings and the namelist array itself.
		for (int i = 0; i < n; i++)
		{
			free(namelist[i]);
		}
		free(namelist);

		return fileName;
	}
	else
	{
		Q_strlcpy(fileName, file);
		Q_strlwr(fileName);
		return fileName;
	}
}

#endif // #if !defined(_WIN32)
