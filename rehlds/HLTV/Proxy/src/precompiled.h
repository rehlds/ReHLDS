#pragma once

#include "basetypes.h"
#include "FileSystem.h"
#include "strtools.h"

#include "interface.h"
#include "IBaseSystem.h"

#include "mem.h"
#include "common.h"

#include "common/md5.h"
#include "common/random.h"
#include "common/byteorder.h"
#include "common/ServerInfo.h"
#include "common/common_hltv.h"
#include "common/net_internal.h"
#include "common/mathlib_internal.h"

#include "common/DirectorCmd.h"
#include "common/NetAddress.h"
#include "common/NetChannel.h"
#include "common/BaseClient.h"

#include "common/DemoFile.h"
#include "common/munge.h"

#include <steam/steam_gameserver.h>
#include "SteamApiCompat.h"

#include <HLTV/IWorld.h>
#include <HLTV/IProxy.h>
#include <HLTV/IServer.h>
#include <HLTV/IClient.h>
#include <HLTV/INetChannel.h>
#include <HLTV/INetwork.h>

// Proxy module stuff
#include "Proxy.h"
#include "ProxyClient.h"
#include "DemoClient.h"
#include "FakeClient.h"

#include "Master.h"
#include "Status.h"
