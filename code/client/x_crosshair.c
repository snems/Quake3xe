#include "client.h"
#include "x_local.h"
#include <float.h>

//TODO: 
// - baseq3 and cpma doesn't work
// - add rotated hit crosshairs

// ====================
//   CVars

//
static cvar_t *x_ch_fix_resolution = 0;
static cvar_t *x_ch_auto_scale = 0;
//
static cvar_t *x_ch_hit_icon = 0;
static cvar_t *x_ch_hit_icon_scale = 0;
static cvar_t *x_ch_hit_lock_icon = 0;
// 
static cvar_t *x_ch_opaque = 0;
static cvar_t *x_ch_color = 0;
static cvar_t *x_ch_rotate45 = 0;
static cvar_t *x_ch_action = 0;
static cvar_t *x_ch_action_color = 0;
// 
static cvar_t *x_ch_decor = 0;
static cvar_t *x_ch_decor_size = 0;
static cvar_t *x_ch_decor_opaque = 0;
static cvar_t *x_ch_decor_color = 0;
static cvar_t *x_ch_decor_rotate45 = 0;
static cvar_t *x_ch_decor_action = 0;
static cvar_t *x_ch_decor_action_color = 0;
//
static cvar_t *cg_crosshairSize = 0;
static cvar_t *cg_drawCrosshair = 0;

// ====================
//   Const vars

static char X_HELP_CH_FIX_RESOLUTION[] = "\n ^fx_ch_fix_resolution^5 0|1^7\n\n"
										 "   Turn on\\off fix for crosshair resolution. The fix removes crosshair deformation on wide-screen resolutions like 16:9.\n";
static char X_HELP_CH_AUTO_SCALE[] = "\n ^fx_ch_auto_scale^5 0|1^7\n\n"
									 "   Crosshair size controlled depends on a distance to the nearest target.\n";

static char X_HELP_CH_HIT_ICON[] = "\n ^fx_ch_hit_icon^5 0|1^7\n\n"
								   "   Show hit icons on a crosshair when you give a damage to an enemy.\n"
								   "   If promode is enabled icons indicate a damage impact.\n";
static char X_HELP_CH_HIT_ICON_SCALE[] = "\n ^fx_ch_hit_icon_scale^5 0.5|...|1.5^7\n\n"
										 "   Change size of the hit icons. \n";
static char X_HELP_CH_HIT_LOCK_ICON[] = "\n ^fx_ch_hit_lock_icon^5 0|...|4^7\n\n"
										"   Force to show a specific hit icon.\n";
static char X_HELP_CH_OPAQUE[] = "\n ^fx_ch_opaque^5 0.0|...|1.0^7\n\n"
								 "   Change a crosshair transparency where 1.0 is opaque and 0.0 is completely transparent.\n";
static char X_HELP_CH_COLOR[] = "\n ^fx_ch_color <color>^7\n\n"
								"   Change a crosshair color, 0 switch a crosshair color to default mode (ch_crosshairColor, etc).\n";
static char X_HELP_CH_ROTATE45[] = "\n ^fx_ch_rotate45^5 0|1^7\n\n"
								   "   Rotate a crosshair to 45 degrees.\n";
static char X_HELP_CH_ACTION[] = "\n ^fx_ch_action^5 0|...|3^7\n\n"
								 "   Enable an effect on a crosshair when specific event happens:\n"
								 "    1 - always visible but pulsates when you hit an enemy\n"
								 "    2 - isn't visible by default but appears when you hit an enemy\n"
								 "    3 - changes color on a hit\n"
								 "   * use ^fx_ch_action_color^7 to control an effect color\n";
static char X_HELP_CH_ACTION_COLOR[] = "\n ^fx_ch_action_color^5 <color>^7\n\n"
									   "   Change a crosshair effect color when an action happens (1-9 or #RRGGBB and #RGB).\n";

static char X_HELP_CH_DECOR[] = "\n ^fx_ch_decor^5 0|...|100^7\n\n"
								"   Display a crosshair decoration as a secondary layer.\n";
static char X_HELP_CH_DECOR_SIZE[] = "\n ^fx_ch_decor_size^5 0|...|100^7\n\n"
									 "   Set a size of the crosshair decoration.\n";
static char X_HELP_CH_DECOR_OPAQUE[] = "\n ^fx_ch_decor_opaque^5 0.0|...|1.0^7\n\n"
									   "   Control a decoration transparency where 1.0 is opaque and 0.0 is completely transparent.\n";
static char X_HELP_CH_DECOR_COLOR[] = "\n ^fx_ch_decor_color^5 <color>^7\n\n"
									  "   Change a decoration color, 0 makes a decoration color the same to a crosshair.\n";//TODO: color format
static char X_HELP_CH_DECOR_ROTATE45[] = "\n ^fx_ch_decor_rotate45^5 0|1^7\n\n"
										 "   Rotate a decoration to 45 degrees.\n";
static char X_HELP_CH_DECOR_ACTION[] = "\n ^fx_ch_decor_action^5 0|...|4^7\n\n"
									   "   Enable action effect for crosshair decoration on a specific event:\n"
									   "    1 - always visible but pulsates when you hit an enemy\n"
									   "    2 - isn't visible by default but appears when you hit an enemy\n"
									   "    3 - changes color on a hit\n"
									   "    4 - changes color when you aimed at an enemy\n"
									   "   * use ^fx_ch_decor_action_color^7 to control an effect color\n";
static char X_HELP_CH_DECOR_ACTION_COLOR[] = "\n ^fx_ch_decor_action_color^5 <color>^7\n\n"
											 "   Change a decoration effect color when an action happens (1-9 or #RRGGBB and #RGB).\n";


// ====================
//   Static routines

static qboolean IsCrosshairShader(qhandle_t shader);
static void DrawCrosshairHitIcon(float x, float y, float w, float h);
static qhandle_t LoadCrosshairWithoutLimit(qhandle_t shader);
static void ChangeCrosshairOnHit(HitCrosshairDamage damage);
static void DrawCustomizedCrosshair(float x, float y, float w, float h, qhandle_t shader);

// ====================
//   Implementation

void X_CH_Init()
{
	HitCrosshairIcon *hc = &xmod.ch.hc;
	hc->dmg = HitNone;
	hc->startMS = 0;
	hc->durationMS = 300; // ms

	HitCrosshairPulse *hp = &xmod.ch.hp;
	hp->durationMS = 0;
	hp->startMS = 0;

	cg_crosshairSize = Cvar_Get("cg_crosshairSize", "24", CVAR_ARCHIVE);
	cg_drawCrosshair = Cvar_Get("cg_drawCrosshair", "4", CVAR_ARCHIVE);

	X_Main_RegisterXCommand(x_ch_fix_resolution, "1", "0", "1", X_HELP_CH_FIX_RESOLUTION);

	X_Main_RegisterXCommand(x_ch_auto_scale, "0", "0", "1", X_HELP_CH_AUTO_SCALE);

	X_Main_RegisterXCommand(x_ch_hit_icon, "0", "0", "1", X_HELP_CH_HIT_ICON);

	X_Main_RegisterFloatXCommand(x_ch_hit_icon_scale, "1.0", "0.5", "1.5", X_HELP_CH_HIT_ICON_SCALE);

	X_Main_RegisterXCommand(x_ch_hit_lock_icon, "0", "0", "4", X_HELP_CH_HIT_LOCK_ICON);

	X_Main_RegisterFloatXCommand(x_ch_opaque, "1.0", "0.0", "1.0", X_HELP_CH_OPAQUE);

	X_Main_RegisterXCommand(x_ch_color, "0", 0, 0, X_HELP_CH_COLOR);

	X_Main_RegisterXCommand(x_ch_rotate45, "0", "0", "1", X_HELP_CH_ROTATE45);

	X_Main_RegisterXCommand(x_ch_action, "3", "0", "3", X_HELP_CH_ACTION);

	X_Main_RegisterXCommand(x_ch_action_color, "#C00", 0, 0, X_HELP_CH_ACTION_COLOR);

	X_Main_RegisterXCommand(x_ch_decor, "0", "0", "100", X_HELP_CH_DECOR);

	X_Main_RegisterXCommand(x_ch_decor_size, "24", "0", "100", X_HELP_CH_DECOR_SIZE);

	X_Main_RegisterFloatXCommand(x_ch_decor_opaque, "1.0", "0.0", "1.0", X_HELP_CH_DECOR_OPAQUE);

	X_Main_RegisterXCommand(x_ch_decor_color, "0", 0, 0, X_HELP_CH_DECOR_COLOR);

	X_Main_RegisterXCommand(x_ch_decor_rotate45, "0", "0", "1", X_HELP_CH_DECOR_ROTATE45);

	X_Main_RegisterXCommand(x_ch_decor_action, "0", "0", "4", X_HELP_CH_DECOR_ACTION);

	X_Main_RegisterXCommand(x_ch_decor_action_color, "1", 0, 0, X_HELP_CH_DECOR_ACTION_COLOR);

	X_Misc_InitCustomColor(x_ch_color, &xmod.ch.front);
	X_Misc_InitCustomColor(x_ch_decor_color, &xmod.ch.decor);

	X_Misc_InitCustomColor(x_ch_action_color, &xmod.ch.actionFront);
	X_Misc_InitCustomColor(x_ch_decor_action_color, &xmod.ch.actionDecor);
}

qboolean X_CH_CustomizeCrosshair(float x, float y, float w, float h, qhandle_t shader)
{
	if (!IsCrosshairShader(shader))
	{
		return qfalse;
	}

	shader = LoadCrosshairWithoutLimit(shader);

	DrawCustomizedCrosshair(x, y, w, h, shader);

	DrawCrosshairHitIcon(x, y, w, h);

	return qtrue;
}

static qboolean IsCrosshairShader(qhandle_t shader)
{
	XModResources *rs = &xmod.rs;

	for (int i = 0; i < countof(*rs->shaderCrosshairs); i++)
	{
		if (rs->shaderCrosshairs[0][i] && rs->shaderCrosshairs[0][i] == shader)
		{
			return qtrue;
		}

		if (rs->shaderCrosshairs[1][i] && rs->shaderCrosshairs[1][i] == shader)
		{
			return qtrue;
		}
	}

	return qfalse;
}

static qhandle_t LoadCrosshairWithoutLimit(qhandle_t shader)
{
	XModResources *rs = &xmod.rs;

	int inx = cg_drawCrosshair->integer;
	if (inx < 10)
	{
		return shader;
	}

	inx -= 10; // Skip default 10 crosshairs

	if (inx >= countof(rs->shaderXCrosshairs))
	{
		return shader;
	}

	if (x_ch_rotate45->integer)
	{
		return (rs->shaderXCrosshairsR45[inx] ? rs->shaderXCrosshairsR45[inx] : shader);
	}

	return (rs->shaderXCrosshairs[inx] ? rs->shaderXCrosshairs[inx] : shader);
}

void X_CH_ChangeCrosshairOnSoundTrigger(const char *soundName)
{
	if (x_ch_hit_icon->integer || x_ch_action->integer || x_ch_decor_action->integer)
	{
		if (!strcmp(soundName, X_SOUND_HIT_LOWEST))
		{
			ChangeCrosshairOnHit(HitLowest);
		}
		else if (!strcmp(soundName, X_SOUND_HIT_LOW))
		{
			ChangeCrosshairOnHit(HitLow);
		}
		else if (!strcmp(soundName, X_SOUND_HIT_MEDIUM))
		{
			ChangeCrosshairOnHit(HitMedium);
		}
		else if (!strcmp(soundName, X_SOUND_HIT_HIGH))
		{
			ChangeCrosshairOnHit(HitHigh);
		}
	}
}

static void ChangeCrosshairOnHit(HitCrosshairDamage damage)
{
	if (x_ch_hit_icon->integer == 1 || x_ch_hit_lock_icon->integer)
	{
		HitCrosshairIcon *hc = &xmod.ch.hc;
		hc->dmg = damage;
		hc->durationMS = 300;
		hc->color[0] = 1.0f;
		hc->color[1] = 0.1f;
		hc->color[2] = 0.1f;
		hc->color[3] = 1.0f; // Alpha

		if (x_ch_hit_lock_icon->integer)
		{
			hc->dmg = x_ch_hit_lock_icon->integer;
		}

		hc->startMS = Sys_Milliseconds();
	}

	if (x_ch_action->integer || x_ch_decor_action->integer)
	{
		xmod.ch.hp.durationMS = 200;
		xmod.ch.hp.startMS = Sys_Milliseconds();
		xmod.ch.hp.increament = 0.4 + (0.1 * damage);
	}
}

static void ScaleCrosshairSize(float *x, float *y, float *w, float *h, int size, float mult)
{
	*x = 640.0 / 2;
	*y = 480.0 / 2;

	*w = (size ? size : *w) * mult;
	*h = (size ? size : *h) * mult;

	SCR_AdjustFrom640(x, y, w, h);

	*x -= *w * 0.5;
	*y -= *h * 0.5;
}

static void DrawCrosshairHitIcon(float x, float y, float w, float h)
{
	HitCrosshairIcon *hc = &xmod.ch.hc;
	XModResources *rs = &xmod.rs;

	if (hc->dmg == HitNone)
	{
		return;
	}

	if (!hc->startMS)
	{
		return;
	}

	int deltaMs = Sys_Milliseconds() - hc->startMS;
	if (deltaMs > hc->durationMS / 2)
	{
		float dur = hc->durationMS / 2.0;
		float step = 1.0 / dur;

		float delta = deltaMs - dur;
		if (delta > dur)
		{
			delta = dur;
		}

		hc->color[3] = 1.0 - (delta * step);
	}
	else
	{
		hc->color[3] = 1.0;
	}

	Original_SetColor(hc->color);

	h = w = cg_crosshairSize->integer;
	ScaleCrosshairSize(&x, &y, &w, &h, 0, x_ch_hit_icon_scale->value);

	int shader = rs->shaderHit[hc->dmg - 1];
	if (shader)
	{
		Original_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, shader);
	}

	if (deltaMs > hc->durationMS)
	{
		hc->startMS = 0;
		hc->dmg = HitNone;
	}
}

static void DrawCrosshairPic(int size, float scale, qhandle_t shader)
{
	float x = 640.0 / 2;
	float y = 480.0 / 2;
	float w, h;

	if (x_ch_fix_resolution->integer)
	{
		SCR_AdjustFrom640(&x, &y, 0, 0);
		h = w = size * 2.4 * scale;
	}
	else
	{
		h = w = size * scale;
		SCR_AdjustFrom640(&x, &y, &w, &h);
	}

	Original_DrawStretchPic(x - (w * 0.5), y - (h * 0.5), w, h, 0, 0, 1, 1, shader);
}

static float GetPulseMultiplier(void)
{
	HitCrosshairPulse *hp = &xmod.ch.hp;

	if (!hp->startMS)
	{
		return 1.0f;
	}

	float inc = xmod.ch.hp.increament;
	float step = inc / hp->durationMS;
	int current = Sys_Milliseconds();
	int delta = current - hp->startMS;

	if (delta > hp->durationMS)
	{
		delta = hp->durationMS;
		xmod.ch.hp.increament = 0.0;
		hp->durationMS = hp->startMS = 0;
	}

	float mult = 1.0f + inc - (delta * step);
	if (mult < 1.0f)
	{
		mult = 1.0f;
	}

	return mult;
}

static float GetPulseOpaqueMultiplier(void)
{
	HitCrosshairPulse *hp = &xmod.ch.hp;

	if (!hp->startMS)
	{
		return 0.0f;
	}

	float inc = 1.0f;
	float step = inc / hp->durationMS;
	int current = Sys_Milliseconds();
	int delta = current - hp->startMS;

	if (delta > hp->durationMS)
	{
		delta = hp->durationMS;
	}

	return inc - (delta * step);
}


static qboolean IsPulseAction(int action)
{
	return (action == 1 || action == 2 ? qtrue : qfalse);
}

static void ChooseCrosshairColor(float *rgba, qboolean active, XCustomColor *color, XCustomColor *actionColor, float opaque)
{
	//TODO: for action 3 and probably other not hidden actions we might make a float color change from actionColor to color
	if (active && X_Misc_IsCustomColorActive(actionColor))
	{
		MAKERGBA(rgba, actionColor->rgb[0], actionColor->rgb[1], actionColor->rgb[2], opaque);
	}
	else if (X_Misc_IsCustomColorActive(color))
	{
		MAKERGBA(rgba, color->rgb[0], color->rgb[1], color->rgb[2], opaque);
	}
	else
	{
		MAKERGBA(rgba, xmod.currentColor[0], xmod.currentColor[1], xmod.currentColor[2], opaque);
	}
}

static qboolean IsActionActive(int action)
{
	if (!action)
	{
		return qfalse;
	}

	if (!xmod.ch.hp.startMS)
	{
		return qfalse;
	}

	return qtrue;
}

static void DrawMainCrosshair(float scale, float pulse, float pulseOpaque, qhandle_t shader)
{
	int action = x_ch_action->integer;
	qboolean active = IsActionActive(action);
	float opaque = x_ch_opaque->value;

	if (active)
	{
		if (IsPulseAction(action))
		{
			scale *= pulse;
		}

		if (action == 2)
		{
			opaque *= pulseOpaque;
		}
	}

	//Fix: crosshair size for custom crosshairs is different to default ones
	if (cg_drawCrosshair->integer > 10)
	{
		scale *= 2.0f;
	}

	float rgba[4];
	ChooseCrosshairColor(rgba, active, &xmod.ch.front, &xmod.ch.actionFront, opaque);
	Original_SetColor(rgba);

	if (action == 2 && !active)
	{
		return;
	}

	DrawCrosshairPic(cg_crosshairSize->integer, scale, shader);
}

static void DrawDecorCrosshair(float scale, float pulse, float pulseOpaque)
{
	if (!x_ch_decor->integer)
	{
		return;
	}

	int action = x_ch_decor_action->integer;
	qboolean active = IsActionActive(action);
	float opaque = x_ch_decor_opaque->value;

	qhandle_t decor = (
	x_ch_decor_rotate45->integer ?
	xmod.rs.shaderDecorsR45[x_ch_decor->integer - 1]
								 : xmod.rs.shaderDecors[x_ch_decor->integer - 1]);

	if (active)
	{
		if (IsPulseAction(action))
		{
			scale *= pulse;
		}

		if (action == 2)
		{
			opaque *= pulseOpaque;
		}
	}

	float rgba[4];
	ChooseCrosshairColor(rgba, active, &xmod.ch.decor, &xmod.ch.actionDecor, opaque);
	Original_SetColor(rgba);

	if (action == 2 && !active)
	{
		return;
	}

	DrawCrosshairPic(x_ch_decor_size->integer, scale, decor);
}

static void DrawCustomizedCrosshair(float x, float y, float w, float h, qhandle_t shader)
{
	float pulse = GetPulseMultiplier();
	float scale = 1.0f;
	float opaque = GetPulseOpaqueMultiplier();

	DrawDecorCrosshair(scale, pulse, opaque);
	DrawMainCrosshair(scale, pulse, opaque, shader);
}

