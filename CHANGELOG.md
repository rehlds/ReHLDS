# [ReHLDS](https://github.com/ReHLDS/ReHLDS) Changelog

**ReHLDS** is a result of reverse engineering of original `HLDS` (build `6152`/`6153`) using `DWARF` debug info embedded into linux version of `HLDS`, `engine_i486.so`.

Along with reverse engineering, a lot of defects and (potential) bugs were found and fixed.

---

## [`3.13.0.788`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.13.0.788) - 2023-07-12

### Added
- Added `SV_AllowPhysent` hook by @justgo97 in [(#951)](ttps://github.com/dreamstalker/ReHLDS/pull/951)
- `GetBonePosition`: Added bone index bounds check
- `GetAttachment`: Added attachment index bounds check
- Added more checks for possible `numleaf` overflow

### Fixed
- `SV_BuildSoundMsg`: fix '`\n`' in args check

### Changed
- Revert "change destinition folder for linux build" by @wopox1337 in [(#977)](https://github.com/dreamstalker/ReHLDS/pull/977)
- Allowed the clients to connect on the server of different game: Client should be use `setinfo _gd <game>`
- Increased limit leafs `MAX_MAP_LEAFS` up to `32767`

## New Contributors
- @justgo97 made their first contribution in [(#951)](https://github.com/dreamstalker/ReHLDS/pull/951)

**Full Changelog**: [3.12.0.780...3.13.0.788](https://github.com/ReHLDS/ReHLDS/compare/3.12.0.780...3.13.0.788)

## [`3.12.0.780`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.12.0.780) - 2022-09-19

### Fixed
- `Netchan_CreateFileFragments`: Fixed a very old and rare bug with dlfile while downloading direct from server, when content of resource size is less than header size first fragment.

### Changed
- `API`: Implement `*_Precache_*`, `ClientPrintf`, `CheckUserInfo` and `AddResource` hooks by @ShadowsAdi in [(#903)](https://github.com/dreamstalker/ReHLDS/pull/903)

## New Contributors
* @ShadowsAdi made their first contribution in [(#903)](https://github.com/dreamstalker/ReHLDS/pull/903)

**Full Changelog**: [3.11.0.779...3.12.0.780](https://github.com/ReHLDS/ReHLDS/compare/3.11.0.779...3.12.0.780)

## [`3.11.0.779`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.11.0.779) - 2022-08-24

### Fixed
- `StripUnprintableWorker` did not count the null terminator [e9045e3](https://github.com/dreamstalker/ReHLDS/commit/e9045e3)
- Very old and rare bug in function `Netchan_CreateFileFragments` with dlfile hang while downloading direct from server, when content of resource size is less than header size first fragment [d76b06d](https://github.com/dreamstalker/ReHLDS/commit/d76b06d)

**Full Changelog**: [3.11.0.777...3.11.0.779](https://github.com/ReHLDS/ReHLDS/compare/3.11.0.777...3.11.0.779)

## [`3.11.0.777`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.11.0.777) - 2022-06-20

### Fixed
* Fixed `null or empty` input string in `COM_LoadFile` (`FS_Open` with input empty string `""` will succeed on some POSIX systems)
 - Resolved  [(#919)](https://github.com/dreamstalker/ReHLDS/issues/919)

**Full Changelog**: [3.11.0.776...3.11.0.777](https://github.com/ReHLDS/ReHLDS/compare/3.11.0.776...3.11.0.777)

## [`3.11.0.776`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.11.0.776) - 2022-04-20

### Fixed
* Fixed typo `ZONE_DYNAMIC_SIZE`

**Full Changelog**: [3.11.0.767...3.11.0.776](https://github.com/ReHLDS/ReHLDS/compare/3.11.0.767...3.11.0.776)

## [`3.11.0.767`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.11.0.767) - 2021-10-25

### Added
* Implement `SV_EmitPings()` hook by @francoromaniello in [(#858)](https://github.com/ReHLDS/ReHLDS/pull/858)
* Implement `Con_Printf()` hook by @francoromaniello in [(#861)](https://github.com/ReHLDS/ReHLDS/pull/861)

### Changed
* `API`: Add hooks `ED_Alloc()` & `ED_Free()`. by @StevenKal in [(#867)](https://github.com/ReHLDS/ReHLDS/pull/867)
* `SV_HullForEntity`: better log in `Sys_Error` by @wopox1337 in [(#843)](https://github.com/ReHLDS/ReHLDS/pull/843)
* Update on grammar/spelling by @mlgpero in [(#865)](https://github.com/ReHLDS/ReHLDS/pull/865)

## New Contributors
* @StevenKal made their first contribution in [(#867)](https://github.com/ReHLDS/ReHLDS/pull/867)
* @francoromaniello made their first contribution in [(#858)](https://github.com/ReHLDS/ReHLDS/pull/858)
* @Urufusan made their first contribution in [(#865)](https://github.com/ReHLDS/ReHLDS/pull/865)

**Full Changelog**: [v3.10.0.761...3.11.0.767](https://github.com/ReHLDS/ReHLDS/compare/v3.10.0.761...3.11.0.767)

## [`3.10.0.760`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.10.0.760) - 2021-06-23

### Changed
- Changed the destination folder for `Linux build` [(#842)](https://github.com/ReHLDS/ReHLDS/pull/842).
- Temporary removed `Windows build`. :warning:

**Full Changelog**: [3.10.0.759...3.10.0.760](https://github.com/ReHLDS/ReHLDS/compare/3.10.0.759...3.10.0.760)


## [`3.10.0.761`](https://github.com/ReHLDS/ReHLDS/releases/tag/v3.10.0.761) - 2021-06-23

### Changed
- Reset `m_bSentNewResponse` to allow new connection when the client goes through the full stage of connection (`cl:connect` -> `sv:S2C_CONNECTION` -> `cl:new` -> `SV_New_f`) 
  - Related [3a9bfb9](https://github.com/ReHLDS/ReHLDS/commit/3a9bfb9)

**Full Changelog**: [3.10.0.760...v3.10.0.761](https://github.com/ReHLDS/ReHLDS/compare/3.10.0.760...v3.10.0.761)

## [`3.10.0.760`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.10.0.760) - 2021-06-23

### Changed
- Changed the destination folder for `Linux build` [(#842)](https://github.com/ReHLDS/ReHLDS/pull/842).
- Temporary removed `Windows build`. :warning:

**Full Changelog**: [3.10.0.759...3.10.0.760](https://github.com/ReHLDS/ReHLDS/compare/3.10.0.759...3.10.0.760)

## [`3.10.0.759`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.10.0.759) - 2021-06-22

### Fixed
- Fixed volume checking in emit sound [(#341)](https://github.com/ReHLDS/ReHLDS/pull/341)
- `static_map.h`: fix lowercase convert [(#806)](https://github.com/ReHLDS/ReHLDS/pull/806)
- `SV_New_f`: Deny new connection twice at a time if user messages are received;
  - `SV_ReadClientMessage`: Fixed empty names on bad read.

### Changed
- `sv_user.cpp`: Small code refactoring [(#810)](https://github.com/ReHLDS/ReHLDS/pull/810)
- `ReHLDS API`: Enhanced IGameClient/IReHLDSServerData/IReHLDSServerStatic interfaces
- `sv_main.cpp`: SV_New_f() uses Q_snprintf() unsafe format. #807 [()](https://github.com/ReHLDS/ReHLDS/pull/807)

**Full Changelog**: [3.9.0.752...3.10.0.759](https://github.com/ReHLDS/ReHLDS/compare/3.9.0.752...3.10.0.759)

## [`3.9.0.752`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.9.0.752) - 2021-06-14

### Added
- `ReHLDS API`: Add GetEntityInit hook [(#832)](https://github.com/ReHLDS/ReHLDS/pull/832)
- Implement CVar `sv_usercmd_custom_random_seed` [(#837)](https://github.com/ReHLDS/ReHLDS/pull/837)

### Fixed
- `HLTV`: Fix crash in ProcessStringCmd [(#838)](https://github.com/ReHLDS/ReHLDS/pull/838)

### Changed
- `SV_ParseMove`, `SV_ParseConsistencyResponse`: check length

**Full Changelog**: [3.8.0.739...3.9.0.752](https://github.com/ReHLDS/ReHLDS/compare/3.8.0.739...3.9.0.752)

## [`3.8.0.739`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.8.0.739) - 2021-04-21

### Added
* Added libraries libm/librt built on `GLIBC` `2.11.1`  [(#827)](https://github.com/ReHLDS/ReHLDS/pull/827)

### Fixed
* `QuaternionSlerp`: Fixed wrong values  [(#763)](https://github.com/ReHLDS/ReHLDS/pull/763)

### Changed
* Updated `Intel C++ Compiler` version `17.0` up to `19.0`

**Full Changelog**: [3.8.0.723...3.8.0.739](https://github.com/ReHLDS/ReHLDS/compare/3.8.0.723...3.8.0.739)

## [`3.8.0.723`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.8.0.723) - 2021-03-23

### Fixed
* `CalcSurfaceExtents:` Fixed a fatal error on some maps due loss of floating point
* `HLTV:` ExecuteString Fix parser  [(#821)](https://github.com/ReHLDS/ReHLDS/pull/821)

### Changed
* `HLTV:` Downgrade GLIBC version

**Full Changelog**: [3.8.0.715...3.8.0.723](https://github.com/ReHLDS/ReHLDS/compare/3.8.0.715...3.8.0.723)

## [`3.8.0.715`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.8.0.715) - 2021-03-18

### Fixed
* Make sure the `timer` is high precision  [(#799)](https://github.com/ReHLDS/ReHLDS/pull/799)

**Full Changelog**: [3.8.0.711...3.8.0.715](https://github.com/ReHLDS/ReHLDS/compare/3.8.0.711...3.8.0.715)

## [`3.8.0.711`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.8.0.711) - 2021-02-06

### Added
* `HLTV`: Added new chatdelay command  [(#777)](https://github.com/ReHLDS/ReHLDS/pull/777)
* `HLTV`: prevent clients from setting userinfo * keys with setinfo command  [(#792)](https://github.com/ReHLDS/ReHLDS/pull/792)
* `Cbuf_Execute`: Add checks commented out for multi-lines  [(#719)](https://github.com/ReHLDS/ReHLDS/pull/719)

### Fixed
* Fixed local-buffer overrun, may undefined behavior with hitboxes blending or crash (reverse-engineering mistake) [722e19d](https://github.com/ReHLDS/ReHLDS/commit/722e19d)
* Fixed dos attack on connection challenges system  [(#802)](https://github.com/ReHLDS/ReHLDS/pull/802)
* Fixed crash `COM_ListMaps`  [(#791)](https://github.com/ReHLDS/ReHLDS/pull/791)

**Full Changelog**: [3.8.0.702...3.8.0.711](https://github.com/ReHLDS/ReHLDS/compare/3.8.0.702...3.8.0.711)

## [`3.8.0.702`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.8.0.702) - 2020-11-09

### Fixed
* Fixed crash `MSG_ReadFloat`

### Changed
* **ReHLDS API:** Implemented `SV_ShouldSendConsistencyList`
* **ReHLDS API:** Bump minor

**Full Changelog**: [3.7.0.698...3.8.0.702](https://github.com/ReHLDS/ReHLDS/compare/3.7.0.698...3.8.0.702)

## [`3.7.0.698`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.7.0.698) - 2020-08-19

### Added
* Graceful shutdown on sigterm  [(#776)](https://github.com/ReHLDS/ReHLDS/pull/776)
  - Add players kick on `SIGINT \ SIGTERM`
  - Add SIGINT & SIGTERM handling linux console

### Changed
* Changed shutdown method

**Full Changelog**: [3.7.0.697...3.7.0.698](https://github.com/ReHLDS/ReHLDS/compare/3.7.0.697...3.7.0.698)

## [`3.7.0.697`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.7.0.697) - 2020-08-10

### Fixed
* **SV_ProcessFile:** Wrap `Con_Printf` in `Con_NetPrintf` to avoid spam in HLDS console

**Full Changelog**: [3.7.0.696...3.7.0.697](https://github.com/ReHLDS/ReHLDS/compare/3.7.0.696...3.7.0.697)

## [`3.7.0.696`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.7.0.696) - 2020-05-18

### Added
* Implement `svc_exec` support in the engine and HLTV [(#737)](https://github.com/ReHLDS/ReHLDS/pull/737)
  - Added `svc_exec` to the list of svc commands in engine
  - Added `svc_exec` support to HLTV code
  - Made the engine code forward-compatible with future svc_* additions
  - Added reserved svc_* slots in the enumerations

**Full Changelog**: [3.7.0.695...3.7.0.696](https://github.com/ReHLDS/ReHLDS/compare/3.7.0.695...3.7.0.696)

## [`3.7.0.695`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.7.0.695) - 2020-04-06

### Fixed
* Vulnerability fix WAD part 2
  - Client-side: Fixed a potential vulnerability from bogus `tempdecal.wad`

**Full Changelog**: [3.7.0.694...3.7.0.695](https://github.com/ReHLDS/ReHLDS/compare/3.7.0.694...3.7.0.695)

## [`3.7.0.694`](https://github.com/ReHLDS/ReHLDS/releases/tag/3.7.0.694) - 2020-03-20

### Fixed
* Vulnerability fix WAD part 1
  - Server-side: Fixed a potential vulnerability from bogus `tempdecal.wad`

**Full Changelog**: [3.7.0.694](https://github.com/ReHLDS/ReHLDS/commits/3.7.0.694)
