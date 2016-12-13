/*
** c_functions.cpp
** Miscellaneous console command helper functions.
**
**---------------------------------------------------------------------------
** Copyright 2016 Rachael Alexanderson
** Copyright 2003-2016 Christoph Oelckers
** Copyright 1998-2016 Randy Heit
** All rights reserved.
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
*/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "version.h"
#include "c_console.h"
#include "c_dispatch.h"

#include "i_system.h"

#include "doomerrors.h"
#include "doomstat.h"
#include "gstrings.h"
#include "s_sound.h"
#include "g_game.h"
#include "g_level.h"
#include "w_wad.h"
#include "g_level.h"
#include "gi.h"
#include "r_defs.h"
#include "d_player.h"
#include "templates.h"
#include "p_local.h"
#include "r_sky.h"
#include "p_setup.h"
#include "cmdlib.h"
#include "d_net.h"
#include "v_text.h"
#include "p_lnspec.h"
#include "v_video.h"
#include "r_utility.h"
#include "r_data/r_interpolate.h"
#include "c_functions.h"

void C_PrintInfo(AActor *target)
{
	if (target->player)
		Printf("Player=%s, ", target->player->userinfo.GetName());
	Printf("Class=%s, Health=%d, Spawnhealth=%d\n",
		target->GetClass()->TypeName.GetChars(),
		target->health,
		target->SpawnHealth());
	PrintMiscActorInfo(target);
}

void C_AimLine(FTranslatedLineTarget *t, bool nonshootable)
{
	P_AimLineAttack(players[consoleplayer].mo,players[consoleplayer].mo->Angles.Yaw, MISSILERANGE, t, 0.,
		(nonshootable) ? ALF_CHECKNONSHOOTABLE|ALF_FORCENOSMART : 0);
}

void C_PrintInv(AActor *target)
{
	AInventory *item;
	int count = 0;

	if (target == NULL)
	{
		Printf("No target found!\n");
		return;
	}

	if (target->player)
		Printf("Inventory for Player '%s':\n", target->player->userinfo.GetName());
	else
		Printf("Inventory for Target '%s':\n", target->GetClass()->TypeName.GetChars());

	for (item = target->Inventory; item != NULL; item = item->Inventory)
	{
		Printf ("    %s #%u (%d/%d)\n", item->GetClass()->TypeName.GetChars(),
			item->InventoryID,
			item->Amount, item->MaxAmount);
		count++;
	}
	Printf ("  List count: %d\n", count);
}

