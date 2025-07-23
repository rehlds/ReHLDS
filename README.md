# ReHLDS [![C/C++ CI](https://github.com/rehlds/ReHLDS/actions/workflows/build.yml/badge.svg)](https://github.com/rehlds/ReHLDS/actions/workflows/build.yml) [![GitHub release (by tag)](https://img.shields.io/github/downloads/rehlds/ReHLDS/latest/total)](https://github.com/rehlds/ReHLDS/releases/latest) ![GitHub all releases](https://img.shields.io/github/downloads/rehlds/ReHLDS/total) [![Percentage of issues still open](http://isitmaintained.com/badge/open/rehlds/ReHLDS.svg)](http://isitmaintained.com/project/rehlds/ReHLDS "Percentage of issues still open") [![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://www.gnu.org/licenses/gpl-3.0)  [![CII Best Practices](https://bestpractices.dev/projects/10929/badge)](https://www.bestpractices.dev/en/projects/10929) <img align="right" src="https://user-images.githubusercontent.com/5860435/111066129-040e5e00-84f0-11eb-9e1f-7a7e8611da2b.png" alt="ReHLDS" />
Reverse-engineered (and bugfixed) HLDS

## What is this?
ReHLDS is a result of reverse engineering of original HLDS (build 6152/6153) using DWARF debug info embedded into linux version of HLDS, engine_i486.so

Along with reverse engineering, a lot of defects and (potential) bugs were found and fixed

You can try playing on one of many servers that are using ReHLDS: [Game Tracker](http://www.gametracker.com/search/?search_by=server_variable&search_by2=sv_version)

> [!TIP]
> ReHLDS linux-releases now is signed via `GPG`, pubkey is: `63547829004f07716f7be4856c32c4282e60fb67` and could be found at [https://keyserver.ubuntu.com/](https://keyserver.ubuntu.com/pks/lookup?search=63547829004f07716f7be4856c32c4282e60fb67+&fingerprint=on&op=index).
>
> How to:
> 1. [Download](https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x63547829004f07716f7be4856c32c4282e60fb67) `63547829004f07716f7be4856c32c4282e60fb67.asc` key
> 2. Import: `gpg --import 63547829004f07716f7be4856c32c4282e60fb67.asc`
> 3. Download release `archive` and `.asc` file.
> 4. Verify: `gpg --verify some-rehlds.zip.asc some-rehlds.zip`.

## Goals of the project
<ul>
<li>Provide more stable (than official) version of Half-Life dedicated server with extended API for mods and plugins</li>
<li>Performance optimizations (use of SSE for vector math for example) is another goal for the future</li>
</ul>

## 🛠 License

ReHLDS is licensed under the [MIT License](./LICENSE).

### License Transition
> [!NOTE]  
> Originally released under [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html), ReHLDS transitioned to the MIT License in July 2025 with the agreement of the core contributors.  
> See [LICENSE-TRANSITION.md](./LICENSE-TRANSITION.md) for details.

## How can use it?
ReHLDS is fully compatible with the official pre-anniversary edition of HLDS (engine version <= 8684) downloaded by steamcmd. All you have to do is to download ReHLDS binaries and replace original swds.dll/engine_i486.so. For windows you can also copy a swds.pdb file with a debug information.

> [!CAUTION]  
> ReHLDS is not compatible with an old 5xxx or below platforms downloaded by hldsupdatetool.

#### Downloading HLDS via steamcmd

```
app_set_config 90 mod cstrike
app_update 90 -beta steam_legacy validate
```

## Downloads
* [Release builds](https://github.com/rehlds/ReHLDS/releases)
* [Dev builds](https://github.com/rehlds/ReHLDS/actions/workflows/build.yml)

ReHLDS binaries require `SSE`, `SSE2` and `SSE3` instruction sets to run and can benefit from `SSE4.1` and `SSE4.2`

<b>Warning!</b> ReHLDS is not binary compatible with original hlds since it's compiled with compilers other than ones used for original hlds.
This means that plugins that do binary code analysis (Orpheu for example) probably will not work with ReHLDS.

## Configuring
<details>
<summary>Click to expand</summary>
<ul>
<li>listipcfgfile &lt;filename&gt; // File for permanent ip bans. Default: listip.cfg
<li>syserror_logfile &lt;filename&gt; // File for the system error log. Default: sys_error.log
<li>sv_auto_precache_sounds_in_models &lt;1|0&gt; // Automatically precache sounds attached to models. Deault: 0
<li>sv_delayed_spray_upload &lt;1|0&gt; // Upload custom sprays after entering the game instead of when connecting. It increases upload speed. Default: 0
<li>sv_echo_unknown_cmd &lt;1|0&gt; // Echo in the console when trying execute an unknown command. Default: 0
<li>sv_rcon_condebug &lt;1|0&gt; // Print rcon debug in the console. Default: 1
<li>sv_force_ent_intersection &lt;1|0&gt; // In a 3-rd party plugins used to force colliding of SOLID_SLIDEBOX entities. Default: 0
<li>sv_rehlds_force_dlmax &lt;1|0&gt; // Force a client's cl_dlmax cvar to 1024. It avoids an excessive packets fragmentation. Default: 0
<li>sv_rehlds_hull_centering &lt;1|0&gt; // Use center of hull instead of corner. Default: 0
<li>sv_rehlds_movecmdrate_max_avg // Max average level of 'move' cmds for ban. Default: 400
<li>sv_rehlds_movecmdrate_avg_punish // Time in minutes for which the player will be banned (0 - Permanent, use a negative number for a kick). Default: 5
<li>sv_rehlds_movecmdrate_max_burst // Max burst level of 'move' cmds for ban. Default: 2500
<li>sv_rehlds_movecmdrate_burst_punish // Time in minutes for which the player will be banned (0 - Permanent, use a negative number for a kick). Default: 5
<li>sv_rehlds_send_mapcycle &lt;1|0&gt; // Send mapcycle.txt in serverinfo message (HLDS behavior, but it is unused on the client). Default: 0
<li>sv_rehlds_stringcmdrate_max_avg // Max average level of 'string' cmds for ban. Default: 80
<li>sv_rehlds_stringcmdrate_avg_punish // Time in minutes for which the player will be banned (0 - Permanent, use a negative number for a kick). Default: 5
<li>sv_rehlds_stringcmdrate_max_burst // Max burst level of 'string' cmds for ban. Default: 400
<li>sv_rehlds_stringcmdrate_burst_punish // Time in minutes for which the player will be banned (0 - Permanent, use a negative number for a kick). Default: 5
<li>sv_rehlds_userinfo_transmitted_fields // Userinfo fields only with these keys will be transmitted to clients via network. If not set then all fields will be transmitted (except prefixed with underscore). Each key must be prefixed by backslash, for example "\name\model\*sid\*hltv\bottomcolor\topcolor". See [wiki](https://github.com/rehlds/ReHLDS/wiki/Userinfo-keys) to collect sufficient set of keys for your server. Default: ""
<li>sv_rehlds_attachedentities_playeranimationspeed_fix // Fixes bug with gait animation speed increase when player has some attached entities (aiments). Can cause animation lags when cl_updaterate is low. Default: 0
<li>sv_rehlds_maxclients_from_single_ip // Limit number of connections at the same time from single IP address, not confuse to already connected players. Default: 5
<li>sv_rehlds_local_gametime &lt;1|0&gt; // A feature of local gametime which decrease "lags" if you run same map for a long time. Default: 0
<li>sv_rehlds_allow_large_sprays &lt;1|0&gt; // Allow larger custom logos than 64x64. Default: 1
<li>sv_use_entity_file // Use custom entity file for a map. Path to an entity file will be "maps/[map name].ent". 0 - use original entities. 1 - use .ent files from maps directory. 2 - use .ent files from maps directory and create new .ent file if not exist.
<li>sv_usercmd_custom_random_seed // When enabled server will populate an additional random seed independent of the client. Default: 0
<li>sv_net_incoming_decompression &lt;1|0&gt; // When enabled server will decompress of incoming compressed file transfer payloads. Default: 1
<li>sv_net_incoming_decompression_max_ratio &lt;0|100&gt; // Sets the max allowed ratio between compressed and uncompressed data for file transfer. (A ratio close to 90 indicates large uncompressed data with low entropy) Default: 80.0
<li>sv_net_incoming_decompression_max_size &lt;16|65536&gt; // Sets the max allowed size for decompressed file transfer data. Default: 65536 bytes
<li>sv_net_incoming_decompression_min_failures &lt;0|10&gt; // Sets the min number of decompression failures required before a player's connection is flagged for potential punishment. Default: 4
<li>sv_net_incoming_decompression_max_failures &lt;0|10&gt; // Sets the max number of decompression failures allowed within a specified time window before action is taken against the player. Default: 10
<li>sv_net_incoming_decompression_min_failuretime: &lt;0.1|10.0&gt; // Sets the min time in secs within which decompression failures are tracked to determine if the player exceeds the failure thresholds. Default: 0.1
<li>sv_net_incoming_decompression_punish // Time in minutes for which the player will be banned for malformed/abnormal bzip2 fragments (0 - Permanent, use a negative number for a kick). Default: -1
<li>sv_tags &lt;comma-delimited string list of tags&gt; // Sets a string defining the "gametags" for this server, this is optional, but if it is set it allows users/scripts to filter in the matchmaking/server-browser interfaces based on the value. Default: ""
<li>sv_filterban &lt;-1|0|1&gt;// Set packet filtering by IP mode. -1 - All players will be rejected without any exceptions. 0 - No checks will happen. 1 - All incoming players will be checked if they're IP banned (if they have an IP filter entry), if they are, they will be kicked. Default: 1
</ul>
</details>

## Commands
<ul>
<li>rescount // Prints the total count of precached resources in the server console
<li>reslist &lt;sound | model | decal | generic | event&gt; // Separately prints the details of the precached resources for sounds, models, decals, generic and events in server console. Useful for managing resources and dealing with the goldsource precache limits.
<li>rcon_adduser &lt;ipaddress/CIDR&gt; // Add a new IP address or CIDR range to RCON user list (This command adds a new IP address to the RCON user list. The specified IP or CIDR range is granted privileged access to server console. Without any Rcon users, access is allowed to anyone with a valid password)</li>
<li>rcon_deluser &lt;ipaddress&gt; {removeAll} // Remove an IP address or CIDR range from RCON user list</li>
<li>rcon_users // List all IP addresses and CIDR ranges in RCON user list</li>
</ul>

## Build instructions
### Checking requirements
There are several software requirements for building ReHLDS:

#### Windows
<pre>
Visual Studio 2015 (C++14 standard) and later
</pre>

#### Linux
<pre>
cmake >= 3.10
GCC >= 4.9.2 (Optional)
ICC >= 15.0.1 20141023 (Optional)
LLVM (Clang) >= 6.0 (Optional)
</pre>

### Building

#### Windows
Use `Visual Studio` to build, open `msvc/ReHLDS.sln` and just select from the solution configurations list `Release Swds` or `Debug Swds`

#### Linux

* Optional options using `build.sh --compiler=[gcc] --jobs=[N] -D[option]=[ON or OFF]` (without square brackets)

<pre>
-c=|--compiler=[icc|gcc|clang]  - Select preferred C/C++ compiler to build
-j=|--jobs=[N]                  - Specifies the number of jobs (commands) to run simultaneously (For faster building)

<sub>Definitions (-D)</sub>
DEBUG                           - Enables debugging mode
USE_STATIC_LIBSTDC              - Enables static linking library libstdc++
</pre>

* ICC          <pre>./build.sh --compiler=intel</pre>
* LLVM (Clang) <pre>./build.sh --compiler=clang</pre>
* GCC          <pre>./build.sh --compiler=gcc</pre>

##### Checking build environment (Debian / Ubuntu)

<details>
<summary>Click to expand</summary>

<ul>
<li>
Installing required packages
<pre>
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib
sudo apt-get install -y build-essential
sudo apt-get install -y libc6-dev libc6-dev-i386
sudo apt-get install -y cmake			
</pre>
</li>

<li>
Select the preferred C/C++ Compiler installation
<pre>
1) sudo apt-get install -y gcc g++
2) sudo apt-get install -y clang
</pre>
</li>
</ul>
</details>

## How can I help the project?
Just install it on your game server and report problems you faced.
Merge requests are also welcome :)
