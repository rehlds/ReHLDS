//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef QLIMITS_H
#define QLIMITS_H

#if defined( _WIN32 )
#pragma once
#endif

// DATA STRUCTURE INFO

#define MAX_NUM_ARGVS		50

// SYSTEM INFO
#define MAX_QPATH		64		// max length of a game pathname
#define MAX_OSPATH		260		// max length of a filesystem pathname

#define ON_EPSILON		0.1		// point on plane side epsilon

#define MAX_LIGHTSTYLE_INDEX_BITS	6
#define MAX_LIGHTSTYLES			(1<<MAX_LIGHTSTYLE_INDEX_BITS)
constexpr auto MAX_LIGHTSTYLE_SIZE = size_t{64};

// Resource counts
#define MAX_MODEL_INDEX_BITS		9	// sent as a short
#define MAX_MODELS			(1<<MAX_MODEL_INDEX_BITS)
#define MAX_SOUND_INDEX_BITS		9
#define MAX_SOUNDS			(1<<MAX_SOUND_INDEX_BITS)
#define MAX_SOUNDS_HASHLOOKUP_SIZE	(MAX_SOUNDS * 2 - 1)

#define MAX_GENERIC_INDEX_BITS		9
#define MAX_GENERIC			(1<<MAX_GENERIC_INDEX_BITS)
#define MAX_DECAL_INDEX_BITS		9
#define MAX_BASE_DECALS			(1<<MAX_DECAL_INDEX_BITS)

#define MAX_EVENTS			256
#define MAX_PACKET_ENTITIES		1024	// 1024 visible entities per frame

#endif // QLIMITS_H
