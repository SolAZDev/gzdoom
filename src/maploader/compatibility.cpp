/*
** compatibility.cpp
** Handles compatibility flags for maps that are unlikely to be updated.
**
**---------------------------------------------------------------------------
** Copyright 2009 Randy Heit
** Copyright 2009-2018 Christoph Oelckers
 All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
** This file is for maps that have been rendered broken by bug fixes or other
** changes that seemed minor at the time, and it is unlikely that the maps
** will be changed. If you are making a map and you know it needs a
** compatibility option to play properly, you are advised to specify so with
** a MAPINFO.
*/

// HEADER FILES ------------------------------------------------------------

#include "sc_man.h"
#include "doomstat.h"
#include "c_dispatch.h"
#include "gi.h"
#include "g_level.h"
#include "p_lnspec.h"
#include "p_tags.h"
#include "w_wad.h"
#include "textures.h"
#include "g_levellocals.h"
#include "vm.h"
#include "actor.h"
#include "p_setup.h"
#include "maploader/maploader.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

union FMD5Holder
{
	uint8_t Bytes[16];
	uint32_t DWords[4];
	hash_t Hash;
};

struct FCompatValues
{
	int CompatFlags[3];
	unsigned int ExtCommandIndex;
};

struct FMD5HashTraits
{
	hash_t Hash(const FMD5Holder key)
	{
		return key.Hash;
	}
	int Compare(const FMD5Holder left, const FMD5Holder right)
	{
		return left.DWords[0] != right.DWords[0] ||
			left.DWords[1] != right.DWords[1] ||
			left.DWords[2] != right.DWords[2] ||
			left.DWords[3] != right.DWords[3];
	}
};

struct FCompatOption
{
	const char *Name;
	uint32_t CompatFlags;
	int WhichSlot;
};

enum
{
	SLOT_COMPAT,
	SLOT_COMPAT2,
	SLOT_BCOMPAT
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

static TMap<FMD5Holder, FCompatValues, FMD5HashTraits> BCompatMap;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static FCompatOption Options[] =
{
	{ "setslopeoverflow",		BCOMPATF_SETSLOPEOVERFLOW, SLOT_BCOMPAT },
	{ "resetplayerspeed",		BCOMPATF_RESETPLAYERSPEED, SLOT_BCOMPAT },
	{ "vileghosts",				BCOMPATF_VILEGHOSTS, SLOT_BCOMPAT },
	{ "ignoreteleporttags",		BCOMPATF_BADTELEPORTERS, SLOT_BCOMPAT },
	{ "rebuildnodes",			BCOMPATF_REBUILDNODES, SLOT_BCOMPAT },
	{ "linkfrozenprops",		BCOMPATF_LINKFROZENPROPS, SLOT_BCOMPAT },
	{ "floatbob",				BCOMPATF_FLOATBOB, SLOT_BCOMPAT },
	{ "noslopeid",				BCOMPATF_NOSLOPEID, SLOT_BCOMPAT },
	{ "clipmidtex",				BCOMPATF_CLIPMIDTEX, SLOT_BCOMPAT },

	// list copied from g_mapinfo.cpp
	{ "shorttex",				COMPATF_SHORTTEX, SLOT_COMPAT },
	{ "stairs",					COMPATF_STAIRINDEX, SLOT_COMPAT },
	{ "limitpain",				COMPATF_LIMITPAIN, SLOT_COMPAT },
	{ "nopassover",				COMPATF_NO_PASSMOBJ, SLOT_COMPAT },
	{ "notossdrops",			COMPATF_NOTOSSDROPS, SLOT_COMPAT },
	{ "useblocking", 			COMPATF_USEBLOCKING, SLOT_COMPAT },
	{ "nodoorlight",			COMPATF_NODOORLIGHT, SLOT_COMPAT },
	{ "ravenscroll",			COMPATF_RAVENSCROLL, SLOT_COMPAT },
	{ "soundtarget",			COMPATF_SOUNDTARGET, SLOT_COMPAT },
	{ "dehhealth",				COMPATF_DEHHEALTH, SLOT_COMPAT },
	{ "trace",					COMPATF_TRACE, SLOT_COMPAT },
	{ "dropoff",				COMPATF_DROPOFF, SLOT_COMPAT },
	{ "boomscroll",				COMPATF_BOOMSCROLL, SLOT_COMPAT },
	{ "invisibility",			COMPATF_INVISIBILITY, SLOT_COMPAT },
	{ "silentinstantfloors",	COMPATF_SILENT_INSTANT_FLOORS, SLOT_COMPAT },
	{ "sectorsounds",			COMPATF_SECTORSOUNDS, SLOT_COMPAT },
	{ "missileclip",			COMPATF_MISSILECLIP, SLOT_COMPAT },
	{ "crossdropoff",			COMPATF_CROSSDROPOFF, SLOT_COMPAT },
	{ "wallrun",				COMPATF_WALLRUN, SLOT_COMPAT },		// [GZ] Added for CC MAP29
	{ "anybossdeath",			COMPATF_ANYBOSSDEATH, SLOT_COMPAT },// [GZ] Added for UAC_DEAD
	{ "mushroom",				COMPATF_MUSHROOM, SLOT_COMPAT },
	{ "mbfmonstermove",			COMPATF_MBFMONSTERMOVE, SLOT_COMPAT },
	{ "corpsegibs",				COMPATF_CORPSEGIBS, SLOT_COMPAT },
	{ "noblockfriends",			COMPATF_NOBLOCKFRIENDS, SLOT_COMPAT },
	{ "spritesort",				COMPATF_SPRITESORT, SLOT_COMPAT },
	{ "hitscan",				COMPATF_HITSCAN, SLOT_COMPAT },
	{ "lightlevel",				COMPATF_LIGHT, SLOT_COMPAT },
	{ "polyobj",				COMPATF_POLYOBJ, SLOT_COMPAT },
	{ "maskedmidtex",			COMPATF_MASKEDMIDTEX, SLOT_COMPAT },
	{ "badangles",				COMPATF2_BADANGLES, SLOT_COMPAT2 },
	{ "floormove",				COMPATF2_FLOORMOVE, SLOT_COMPAT2 },
	{ "soundcutoff",			COMPATF2_SOUNDCUTOFF, SLOT_COMPAT2 },
	{ "pointonline",			COMPATF2_POINTONLINE, SLOT_COMPAT2 },
	{ "multiexit",				COMPATF2_MULTIEXIT, SLOT_COMPAT2 },
	{ "teleport",				COMPATF2_TELEPORT, SLOT_COMPAT2 },
	{ "disablepushwindowcheck",	COMPATF2_PUSHWINDOW, SLOT_COMPAT2 },
	{ NULL, 0, 0 }
};

static const char *const LineSides[] =
{
	"Front", "Back", NULL
};

static const char *const WallTiers[] =
{
	"Top", "Mid", "Bot", NULL
};

static const char *const SectorPlanes[] =
{
	"floor", "ceil", NULL
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
// ParseCompatibility
//
//==========================================================================

void ParseCompatibility()
{
	TArray<FMD5Holder> md5array;
	FMD5Holder md5;
	FCompatValues flags;
	int i, x;
	unsigned int j;

	BCompatMap.Clear();

	// The contents of this file are not cumulative, as it should not
	// be present in user-distributed maps.
	FScanner sc(Wads.GetNumForFullName("compatibility.txt"));

	while (sc.GetString())	// Get MD5 signature
	{
		do
		{
			if (strlen(sc.String) != 32)
			{
				sc.ScriptError("MD5 signature must be exactly 32 characters long");
			}
			for (i = 0; i < 32; ++i)
			{
				if (sc.String[i] >= '0' && sc.String[i] <= '9')
				{
					x = sc.String[i] - '0';
				}
				else
				{
					sc.String[i] |= 'a' ^ 'A';
					if (sc.String[i] >= 'a' && sc.String[i] <= 'f')
					{
						x = sc.String[i] - 'a' + 10;
					}
					else
					{
						x = 0;
						sc.ScriptError("MD5 signature must be a hexadecimal value");
					}
				}
				if (!(i & 1))
				{
					md5.Bytes[i / 2] = x << 4;
				}
				else
				{
					md5.Bytes[i / 2] |= x;
				}
			}
			md5array.Push(md5);
			sc.MustGetString();
		} while (!sc.Compare("{"));
		memset(flags.CompatFlags, 0, sizeof(flags.CompatFlags));
		flags.ExtCommandIndex = ~0u;
		while (sc.GetString())
		{
			if ((i = sc.MatchString(&Options[0].Name, sizeof(*Options))) >= 0)
			{
				flags.CompatFlags[Options[i].WhichSlot] |= Options[i].CompatFlags;
			}
			else
			{
				sc.UnGet();
				break;
			}
		}
		sc.MustGetStringName("}");
		for (j = 0; j < md5array.Size(); ++j)
		{
			BCompatMap[md5array[j]] = flags;
		}
		md5array.Clear();
	}
}

//==========================================================================
//
// CheckCompatibility
//
//==========================================================================

FName MapLoader::CheckCompatibility(MapData *map)
{
	FMD5Holder md5;
	FCompatValues *flags;

	if (BCompatMap.CountUsed() == 0) ParseCompatibility();

	ii_compatflags = 0;
	ii_compatflags2 = 0;
	ib_compatflags = 0;

	// When playing Doom IWAD levels force COMPAT_SHORTTEX and COMPATF_LIGHT.
	// I'm not sure if the IWAD maps actually need COMPATF_LIGHT but it certainly does not hurt.
	// TNT's MAP31 also needs COMPATF_STAIRINDEX but that only gets activated for TNT.WAD.
	if (Wads.GetLumpFile(map->lumpnum) == Wads.GetIwadNum() && (gameinfo.flags & GI_COMPATSHORTTEX) && Level->maptype == MAPTYPE_DOOM)
	{
		ii_compatflags = COMPATF_SHORTTEX|COMPATF_LIGHT;
		if (gameinfo.flags & GI_COMPATSTAIRS) ii_compatflags |= COMPATF_STAIRINDEX;
	}

	map->GetChecksum(md5.Bytes);

	flags = BCompatMap.CheckKey(md5);

	FString hash;

	for (size_t j = 0; j < sizeof(md5.Bytes); ++j)
	{
		hash.AppendFormat("%02X", md5.Bytes[j]);
	}

	if (developer >= DMSG_NOTIFY)
	{
		Printf("MD5 = %s", hash.GetChars());
		if (flags != NULL)
		{
			Printf(", cflags = %08x, cflags2 = %08x, bflags = %08x\n",
				flags->CompatFlags[SLOT_COMPAT], flags->CompatFlags[SLOT_COMPAT2], flags->CompatFlags[SLOT_BCOMPAT]);
		}
		else
		{
			Printf("\n");
		}
	}

	if (flags != NULL)
	{
		ii_compatflags |= flags->CompatFlags[SLOT_COMPAT];
		ii_compatflags2 |= flags->CompatFlags[SLOT_COMPAT2];
		ib_compatflags |= flags->CompatFlags[SLOT_BCOMPAT];
	}

	// Reset i_compatflags
	compatflags.Callback();
	compatflags2.Callback();
	// Set floatbob compatibility for all maps with an original Hexen MAPINFO.
	if (Level->flags2 & LEVEL2_HEXENHACK)
	{
		ib_compatflags |= BCOMPATF_FLOATBOB;
	}
	return FName(hash, true);	// if this returns NAME_None it means there is no scripted compatibility handler.
}

//==========================================================================
//
// SetCompatibilityParams
//
//==========================================================================

class DLevelCompatibility : public DObject
{
	DECLARE_ABSTRACT_CLASS(DLevelCompatibility, DObject)
public:
	MapLoader *loader;
	FLevelLocals *Level;
};
IMPLEMENT_CLASS(DLevelCompatibility, true, false);


void MapLoader::SetCompatibilityParams(FName checksum)
{
	if (checksum != NAME_None)
	{
		auto lc = Create<DLevelCompatibility>();
		lc->loader = this;
		lc->Level = Level;
		for(auto cls : PClass::AllClasses)
		{
			if (cls->IsDescendantOf(RUNTIME_CLASS(DLevelCompatibility)))
			{
				PFunction *const func = dyn_cast<PFunction>(cls->FindSymbol("Apply", false));
				if (func != nullptr)
				{
					VMValue param[] = { lc, (int)checksum };
					VMCall(func->Variants[0].Implementation, param, 2, nullptr, 0);
				}
			}
		}
	}
}

DEFINE_ACTION_FUNCTION(DLevelCompatibility, OffsetSectorPlane)
{
	PARAM_SELF_PROLOGUE(DLevelCompatibility);
	PARAM_INT(sector);
	PARAM_INT(planeval);
	PARAM_FLOAT(delta);

	sector_t *sec = &self->Level->sectors[sector];
	secplane_t& plane = sector_t::floor == planeval? sec->floorplane : sec->ceilingplane;
	plane.ChangeHeight(delta);
	sec->ChangePlaneTexZ(planeval, delta);
	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelCompatibility, ClearSectorTags)
{
	PARAM_SELF_PROLOGUE(DLevelCompatibility);
	PARAM_INT(sector);
	tagManager.RemoveSectorTags(sector);
	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelCompatibility, AddSectorTag)
{
	PARAM_SELF_PROLOGUE(DLevelCompatibility);
	PARAM_INT(sector);
	PARAM_INT(tag);
	tagManager.AddSectorTag(sector, tag);
	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelCompatibility, SetThingSkills)
{
	PARAM_SELF_PROLOGUE(DLevelCompatibility);
	PARAM_INT(thing);
	PARAM_INT(skillmask);

	if ((unsigned)thing < self->loader->MapThingsConverted.Size())
	{
		self->loader->MapThingsConverted[thing].SkillFilter = skillmask;
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelCompatibility, SetThingXY)
{
	PARAM_SELF_PROLOGUE(DLevelCompatibility);
	PARAM_INT(thing);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);

	if ((unsigned)thing < self->loader->MapThingsConverted.Size())
	{
		auto& pos = self->loader->MapThingsConverted[thing].pos;
		pos.X = x;
		pos.Y = y;
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelCompatibility, SetThingZ)
{
	PARAM_SELF_PROLOGUE(DLevelCompatibility);
	PARAM_INT(thing);
	PARAM_FLOAT(z);

	if ((unsigned)thing < self->loader->MapThingsConverted.Size())
	{
		self->loader->MapThingsConverted[thing].pos.Z = z;
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelCompatibility, SetThingFlags)
{
	PARAM_SELF_PROLOGUE(DLevelCompatibility);
	PARAM_INT(thing);
	PARAM_INT(flags);

	if ((unsigned)thing < self->loader->MapThingsConverted.Size())
	{
		self->loader->MapThingsConverted[thing].flags = flags;
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelCompatibility, SetVertex)
{
	PARAM_SELF_PROLOGUE(DLevelCompatibility);
	PARAM_UINT(vertex);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);

	if (vertex < self->Level->vertexes.Size())
	{
		self->Level->vertexes[vertex].p = DVector2(x, y);
	}
	self->loader->ForceNodeBuild = true;
	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelCompatibility, SetLineSectorRef)
{
	PARAM_SELF_PROLOGUE(DLevelCompatibility);
	PARAM_UINT(lineidx);
	PARAM_UINT(sideidx);
	PARAM_UINT(sectoridx);

	if (   sideidx < 2
		&& lineidx < self->Level->lines.Size()
		&& sectoridx < self->Level->sectors.Size())
	{
		line_t *line = &self->Level->lines[lineidx];
		side_t *side = line->sidedef[sideidx];
		side->sector = &self->Level->sectors[sectoridx];
		if (sideidx == 0) line->frontsector = side->sector;
		else line->backsector = side->sector;
	}
	self->loader->ForceNodeBuild = true;
	return 0;
}

DEFINE_ACTION_FUNCTION(DLevelCompatibility, GetDefaultActor)
{
	PARAM_SELF_PROLOGUE(DLevelCompatibility);
	PARAM_NAME(actorclass);
	ACTION_RETURN_OBJECT(GetDefaultByName(actorclass));
}


DEFINE_FIELD(DLevelCompatibility, Level);

//==========================================================================
//
// CCMD mapchecksum
//
//==========================================================================

CCMD (mapchecksum)
{
	MapData *map;
	uint8_t cksum[16];

	if (argv.argc() < 2)
	{
		Printf("Usage: mapchecksum <map> ...\n");
	}
	for (int i = 1; i < argv.argc(); ++i)
	{
		map = P_OpenMapData(argv[i], true);
		if (map == NULL)
		{
			Printf("Cannot load %s as a map\n", argv[i]);
		}
		else
		{
			map->GetChecksum(cksum);
			const char *wadname = Wads.GetWadName(Wads.GetLumpFile(map->lumpnum));
			delete map;
			for (size_t j = 0; j < sizeof(cksum); ++j)
			{
				Printf("%02X", cksum[j]);
			}
			Printf(" // %s %s\n", wadname, argv[i]);
		}
	}
}

//==========================================================================
//
// CCMD hiddencompatflags
//
//==========================================================================

CCMD (hiddencompatflags)
{
	Printf("%08x %08x %08x\n", ii_compatflags, ii_compatflags2, ib_compatflags);
}

