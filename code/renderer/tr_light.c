/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_light.c

#include "tr_local.h"

#define	DLIGHT_AT_RADIUS		16
// at the edge of a dlight's influence, this amount of light will be added

#define	DLIGHT_MINIMUM_RADIUS	16		
// never calculate a range less than this to prevent huge light numbers

typedef struct {
  dlight_t *dl;
  float power;
  vec3_t origin;
} incidentLight_t;

typedef struct {
  int dlightMap;
  int allocated[128];
  byte lightmap_buffer[65536];
  byte* srcBase;
  byte* dstBase;
  incidentLight_t lights[32];
  int numLights;
} dlightInfo_t;

dlightInfo_t dli;

void R_SetupEntityLightingGrid(trRefEntity_t* ent);

/*
===============
R_TransformDlights

Transforms the origins of an array of dlights.
Used by both the front end (for DlightBmodel) and
the back end (before doing the lighting calculation)
===============
*/
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *ori) {
	int		i;
	vec3_t	temp;

	for ( i = 0 ; i < count ; i++, dl++ ) {
		VectorSubtract( dl->origin, ori->origin, temp );
		dl->transformed[0] = DotProduct( temp, ori->axis[0] );
		dl->transformed[1] = DotProduct( temp, ori->axis[1] );
		dl->transformed[2] = DotProduct( temp, ori->axis[2] );
	}
}

/*
=============
R_DlightBmodel

Determine which dynamic lights may effect this bmodel
=============
*/
void R_DlightBmodel( bmodel_t *bmodel ) {
	int			i, j;
	dlight_t	*dl;
	int			mask;
	msurface_t	*surf;

	// transform all the lights
	R_TransformDlights( tr.refdef.num_dlights, tr.refdef.dlights, &tr.ori );

	mask = 0;
	for ( i=0 ; i<tr.refdef.num_dlights ; i++ ) {
		dl = &tr.refdef.dlights[i];

		// see if the point is close enough to the bounds to matter
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( dl->transformed[j] - bmodel->bounds[1][j] > dl->radius ) {
				break;
			}
			if ( bmodel->bounds[0][j] - dl->transformed[j] > dl->radius ) {
				break;
			}
		}
		if ( j < 3 ) {
			continue;
		}

		// we need to check this light
		mask |= 1 << i;
	}

	tr.currentEntity->needDlights = (mask != 0);

	// set the dlight bits in all the surfaces
	for ( i = 0 ; i < bmodel->numSurfaces ; i++ ) {
		surf = bmodel->firstSurface + i;

		if ( *surf->data == SF_FACE ) {
			((srfSurfaceFace_t *)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		} else if ( *surf->data == SF_GRID ) {
			((srfGridMesh_t *)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		} else if ( *surf->data == SF_TRIANGLES ) {
			((srfTriangles_t *)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		}
	}
}

static byte* R_GetLightGridPalettedColor(int iColor)
{
	// FIXME: unimplemented
}

void R_GetLightingGridValue(const vec3_t vPos, vec3_t vLight)
{
	if (!tr.world || !tr.world->lightGridData || !tr.world->lightGridOffsets) {
		vLight[0] = vLight[1] = vLight[2] = tr.identityLightByte;
	}

	vLight[0] = vLight[1] = vLight[2] = tr.identityLightByte;
	// FIXME: unimplemented
}

void R_GetLightingGridValueFast(const vec3_t vPos, vec3_t vLight)
{
    // FIXME: unimplemented
    VectorSet(vLight, 1.f, 1.f, 1.f);
}

void R_GetLightingForDecal(vec3_t vLight, vec3_t vFacing, vec3_t vOrigin)
{
    // FIXME: unimplemented
    VectorSet(vLight, 1.f, 1.f, 1.f);
}

void R_GetLightingForSmoke(vec3_t vLight, vec3_t vOrigin)
{
    // FIXME: unimplemented
	VectorSet(vLight, 1.f, 1.f, 1.f);
}

static int RB_GetEntityGridLighting()
{
	int iColor;
	int i;
	dlight_t* dl;
	float power;
	vec3_t vLight;
	vec3_t dir;
	float d;
	float* lightOrigin;

	lightOrigin = backEnd.currentSphere->traceOrigin;
	if (!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL) && tr.world->lightGridData) {
		R_GetLightingGridValue(backEnd.currentSphere->traceOrigin, vLight);
	} else {
		vLight[0] = vLight[1] = vLight[2] = tr.identityLight * 150.0;
	}

	for (i = 0; i < backEnd.refdef.num_dlights; i++) {
		dl = &backEnd.refdef.dlights[i];
		VectorSubtract(dl->origin, lightOrigin, dir);
		d = VectorLengthSquared(dir);

		power = dl->radius * dl->radius;
		if (power >= d) {
			d = dl->radius * 7500.0 / d;
			VectorMA(vLight, d, dl->color, vLight);
		}
	}

	if (tr.overbrightShift)
	{
		vLight[0] = tr.overbrightMult * vLight[0];
		vLight[1] = tr.overbrightMult * vLight[1];
		vLight[2] = tr.overbrightMult * vLight[2];
	}

	// normalize
	if (vLight[0] < 255.0 || vLight[1] < 255.0 || vLight[2] < 255.0) {
		float scale = 255.0 / fmin(vLight[0], fmin(vLight[1], vLight[2]));
		VectorScale(vLight, scale, vLight);
	}

	// clamp ambient
	for (i = 0; i < 3; i++) {
		if (vLight[i] > tr.identityLightByte) {
			vLight[i] = tr.identityLightByte;
		}
	}

	// save out the byte packet version
	((byte*)&iColor)[0] = myftol(vLight[0]);
	((byte*)&iColor)[1] = myftol(vLight[1]);
	((byte*)&iColor)[2] = myftol(vLight[2]);
	((byte*)&iColor)[3] = 0xff;

	return iColor;

#if 0
	int				i;
	dlight_t* dl;
	float			power;
	vec3_t			dir;
	float			d;
	vec3_t			lightDir;
	vec3_t			lightOrigin;
	int				ambientlightInt = 0;
	trRefEntity_t	*ent = backEnd.currentEntity;
	trRefdef_t		*refdef = &backEnd.refdef;

	//
	// trace a sample point down to find ambient light
	//
	if (ent->e.renderfx & RF_LIGHTING_ORIGIN) {
		// seperate lightOrigins are needed so an object that is
		// sinking into the ground can still be lit, and so
		// multi-part models can be lit identically
		VectorCopy(ent->e.lightingOrigin, lightOrigin);
	}
	else {
		VectorCopy(ent->e.origin, lightOrigin);
	}

	// if NOWORLDMODEL, only use dynamic lights (menu system, etc)
	if (!(refdef->rdflags & RDF_NOWORLDMODEL)
		&& tr.world->lightGridData) {
		R_SetupEntityLightingGrid(ent);
	}
	else {
		ent->ambientLight[0] = ent->ambientLight[1] =
			ent->ambientLight[2] = tr.identityLight * 150;
		ent->directedLight[0] = ent->directedLight[1] =
			ent->directedLight[2] = tr.identityLight * 150;
		VectorCopy(tr.sunDirection, ent->lightDir);
	}

	// bonus items and view weapons have a fixed minimum add
	if (1 /* ent->e.renderfx & RF_MINLIGHT */) {
		// give everything a minimum light add
		ent->ambientLight[0] += tr.identityLight * 32;
		ent->ambientLight[1] += tr.identityLight * 32;
		ent->ambientLight[2] += tr.identityLight * 32;
	}

	//
	// modify the light by dynamic lights
	//
	d = VectorLength(ent->directedLight);
	VectorScale(ent->lightDir, d, lightDir);

	for (i = 0; i < refdef->num_dlights; i++) {
		dl = &refdef->dlights[i];
		VectorSubtract(dl->origin, lightOrigin, dir);
		d = VectorNormalize(dir);

		power = DLIGHT_AT_RADIUS * (dl->radius * dl->radius);
		if (d < DLIGHT_MINIMUM_RADIUS) {
			d = DLIGHT_MINIMUM_RADIUS;
		}
		d = power / (d * d);

		VectorMA(ent->directedLight, d, dl->color, ent->directedLight);
		VectorMA(lightDir, d, dir, lightDir);
	}

	// clamp ambient
	for (i = 0; i < 3; i++) {
		if (ent->ambientLight[i] > tr.identityLightByte) {
			ent->ambientLight[i] = tr.identityLightByte;
		}
	}

	// save out the byte packet version
	((byte*)&ambientlightInt)[0] = myftol(ent->ambientLight[0]);
	((byte*)&ambientlightInt)[1] = myftol(ent->ambientLight[1]);
	((byte*)&ambientlightInt)[2] = myftol(ent->ambientLight[2]);
	((byte*)&ambientlightInt)[3] = 0xff;

	// transform the direction to local space
	VectorNormalize(lightDir);
	ent->lightDir[0] = DotProduct(lightDir, ent->e.axis[0]);
	ent->lightDir[1] = DotProduct(lightDir, ent->e.axis[1]);
	ent->lightDir[2] = DotProduct(lightDir, ent->e.axis[2]);

	return ambientlightInt;
#endif
	// FIXME: unimplemented
}

void RB_SetupEntityGridLighting()
{
	trRefEntity_t* ent;
	int iColor;

	if (backEnd.currentEntity->bLightGridCalculated) {
		return;
	}

	for (ent = backEnd.currentEntity; ent->e.parentEntity != ENTITYNUM_NONE; ent = &backEnd.refdef.entities[ent->e.parentEntity])
	{
		trRefEntity_t* newref = &backEnd.refdef.entities[ent->e.parentEntity];
		if (newref == ent) {
			assert(!"backEnd.refdef.entities[ent->e.parentEntity] refers to itself\n");
			iColor = newref->iGridLighting;
			break;
		}

		if (newref->bLightGridCalculated) {
			iColor = newref->iGridLighting;
			break;
		}
	}

	if (ent->e.parentEntity == ENTITYNUM_NONE) {
		iColor = RB_GetEntityGridLighting();
	}

	ent = backEnd.currentEntity;
	for (;;)
	{
		ent->bLightGridCalculated = 1;
		ent->iGridLighting = iColor;
		if (ent->e.parentEntity == ENTITYNUM_NONE) {
			break;
		}

		if (ent == &backEnd.refdef.entities[ent->e.parentEntity]) {
			assert(!"backEnd.refdef.entities[ent->e.parentEntity] refers to itself\n");
			break;
		}

		ent = &backEnd.refdef.entities[ent->e.parentEntity];
	}
}

void RB_SetupStaticModelGridLighting(trRefdef_t* refdef, cStaticModelUnpacked_t* ent, const vec3_t lightOrigin)
{
	int iColor;
	int i;
	dlight_t* dl;
	float power;
	vec3_t vLight;
	vec3_t dir;
	float d;

	if (ent->bLightGridCalculated) {
		return;
	}
	ent->bLightGridCalculated = qtrue;

	if (!(refdef->rdflags & RDF_NOWORLDMODEL) && tr.world->lightGridData) {
		R_GetLightingGridValue(backEnd.currentSphere->traceOrigin, vLight);
	}
	else {
		vLight[0] = vLight[1] = vLight[2] = tr.identityLight * 150.0;
	}

	for (i = 0; i < refdef->num_dlights; i++) {
		dl = &refdef->dlights[i];
		VectorSubtract(dl->origin, lightOrigin, dir);
		d = VectorLengthSquared(dir);

		power = dl->radius * dl->radius;
		if (power >= d) {
			d = dl->radius * 7500.0 / d;
			VectorMA(vLight, d, dl->color, vLight);
		}
	}

	if (tr.overbrightShift)
	{
		vLight[0] = tr.overbrightMult * vLight[0];
		vLight[1] = tr.overbrightMult * vLight[1];
		vLight[2] = tr.overbrightMult * vLight[2];
	}

	// normalize
	if (vLight[0] < 255.0 || vLight[1] < 255.0 || vLight[2] < 255.0) {
		float scale = 255.0 / fmin(vLight[0], fmin(vLight[1], vLight[2]));
		VectorScale(vLight, scale, vLight);
	}

	// clamp ambient
	for (i = 0; i < 3; i++) {
		if (vLight[i] > tr.identityLightByte) {
			vLight[i] = tr.identityLightByte;
		}
	}

	// save out the byte packet version
	((byte*)&ent->iGridLighting)[0] = myftol(vLight[0]);
	((byte*)&ent->iGridLighting)[1] = myftol(vLight[1]);
	((byte*)&ent->iGridLighting)[2] = myftol(vLight[2]);
	((byte*)&ent->iGridLighting)[3] = 0xff;
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

extern	cvar_t	*r_ambientScale;
extern	cvar_t	*r_directedScale;

/*
===============
LogLight
===============
*/
static void LogLight( trRefEntity_t *ent ) {
	int	max1, max2;

	if ( !(ent->e.renderfx & RF_FIRST_PERSON ) ) {
		return;
	}

	max1 = ent->ambientLight[0];
	if ( ent->ambientLight[1] > max1 ) {
		max1 = ent->ambientLight[1];
	} else if ( ent->ambientLight[2] > max1 ) {
		max1 = ent->ambientLight[2];
	}

	max2 = ent->directedLight[0];
	if ( ent->directedLight[1] > max2 ) {
		max2 = ent->directedLight[1];
	} else if ( ent->directedLight[2] > max2 ) {
		max2 = ent->directedLight[2];
	}

	ri.Printf( PRINT_ALL, "amb:%i  dir:%i\n", max1, max2 );
}

void R_ClearRealDlights() {
	memset(dli.allocated, 0, sizeof(dli.allocated));
	dli.dlightMap = 0;
}

void R_UploadDlights() {
	int i, h;

	if (!tr.pc.c_dlightSurfaces) {
		return;
	}

	h = 0;
	for (i = 0; i < 128; ++i)
	{
		if (h < dli.allocated[i]) {
			h = dli.allocated[i];
		}
	}

	if (h)
	{
		if (h > 128) {
			ri.Error(ERR_DROP, "R_UploadDlights: bad allocated height");
		}

		GL_Bind(tr.dlightImages[dli.dlightMap]);
		qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, h, GL_RGBA, GL_UNSIGNED_BYTE, dli.lightmap_buffer);

		tr.pc.c_dlightMaps++;
		memset(dli.allocated, 0, sizeof(dli.allocated));
	}
}

qboolean R_AllocLMBlock(int w, int h, int* x, int* y) {
	int i, j;
	int best, best2;

	for (;;)
	{
		best = 128;
		for (i = 0; i < 128 - w; i++)
		{
			best2 = 0;

			for (j = 0; j < w && dli.allocated[i] < best; j++)
			{
				if (best2 < dli.allocated[i]) {
					best2 = dli.allocated[i];
				}
			}

			if (j == w)
			{
				*x = i;
				*y = best2;
				best = best2;
			}
		}

		if (h + best <= 128) {
			break;
		}

		if (dli.dlightMap == 14) {
			return qfalse;
		}

		R_UploadDlights();
		dli.dlightMap++;
	}

	return qtrue;
}
