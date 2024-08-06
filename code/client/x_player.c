#include "client.h"
#include "x_local.h"

// ====================
//   CVars

static cvar_t *x_ps_dead_body_black = 0;

static cvar_t *x_ps_auto_revival = 0;

// ====================
//   Const vars
//

static char X_HELP_PS_DEAD_BODY_BLACK[] = "\n ^fx_ps_dead_body_black^5 0|...|3^7\n\n"
										  "   Make dead and frozen models dark.\n"
										  "    1 - light gray\n"
										  "    2 - gray\n"
										  "    3 - dark gray\n";

// ====================
//   Static routines


static void MakeDeadBodyBlack(PlayerModel model, refEntity_t *ref);

// ====================
//   Implementation

void X_PS_Init()
{
	X_Main_RegisterXCommand(x_ps_dead_body_black, "2", "0", "3", X_HELP_PS_DEAD_BODY_BLACK);
	X_Main_RegisterXCommand(x_ps_auto_revival, "1", "0", "1", 0);//TODO:
}

void X_PS_CustomizePlayerModel(refEntity_t *ref)
{
	PlayerModel model = X_Team_IsPlayerModel(ref->hModel);

	if (model == NotPlayer)
	{
		return;
	}

	MakeDeadBodyBlack(model, ref);
}

void X_PS_AutoRevival(snapshot_t *snapshot)
{
	if (!x_ps_auto_revival->integer)
	{
		return;
	}

	if (snapshot->ps.clientNum != clc.clientNum)
	{
		return;
	}

	if (snapshot->ps.pm_type != PM_DEAD)
	{
		return;
	}

	if (xmod.snap.ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if ((xmod.gs.type == GameTDM && xmod.gs.freezetag) || xmod.gs.type == GameCTF
		|| xmod.gs.type == Game1v1 || xmod.gs.type == GameFFA)
	{
		// Semulate mouse1 click
		
		Cmd_ExecuteString("+attack");
		Cmd_ExecuteString("-attack");
	}
}

static void MakeDeadBodyBlack(PlayerModel model, refEntity_t *ref)
{
	if (xmod.gs.mode == ModeCPMA)
	{
		return;
	}

	if (!x_ps_dead_body_black->integer)
	{
		return;
	}

	if (xmod.deadPlayerParts == NotPlayer && model == LegsModel)
	{
		qboolean found = qfalse;

		if (xmod.gs.freezetag)
		{
			int client = X_GS_GetClientIDByOrigin(ref->origin);
			if (client >= 0 && xmod.gs.ps[client].powerups & (1 << PW_BATTLESUIT) && xmod.gs.ps[client].entity >= MAX_CLIENTS)
			{
				xmod.deadPlayerParts = model;
				found = qtrue;
			}
		}

		if (!found && X_GS_IsClientDeadByOrigin(ref->origin))
		{
			xmod.deadPlayerParts = model;
		}
	}

	if (xmod.deadPlayerParts != NotPlayer)
	{
		ref->shader.rgba[0] = ref->shader.rgba[1] = ref->shader.rgba[2] = 220 - (55 * x_ps_dead_body_black->integer);
		xmod.deadPlayerParts = (model == HeadModel ? NotPlayer : model);
	}
}
