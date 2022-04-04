//-----------------------------------------------------------------------------
//
//  $Logfile:: /fakk2_code/fakk2_new/cgame/tr_types.h                         $
// $Revision:: 24                                                             $
//   $Author:: Markd                                                          $
//     $Date:: 6/28/00 7:34p                                                  $
//
// Copyright (C) 1998 by Ritual Entertainment, Inc.
// All rights reserved.
//
// This source may not be distributed and/or modified without
// expressly written permission by Ritual Entertainment, Inc.
//
// $Log:: /fakk2_code/fakk2_new/cgame/tr_types.h                              $
// 
// 24    6/28/00 7:34p Markd
// Increased EXTENSIONS string size
// 
// 23    6/20/00 11:55a Markd
// changed frames from bytes to shorts
// 
// 22    3/31/00 11:45a Markd
// Added skyportal toggling support
// 
// 21    3/17/00 6:37p Markd
// fixed 8 bit anim bug
// 
// 20    3/04/00 2:47p Markd
// Made dynamic light types into bits so that they could be combined
// 
// 19    2/25/00 12:31p Markd
// Added additive dynamic light support
// 
// 18    2/21/00 7:02p Markd
// started adding old-school dlights and sky_alpha support
// 
// 17    2/14/00 5:41p Jimdose
// added uselegs to refEntity_t
// 
// 16    12/11/99 3:37p Markd
// q3a gold checkin, first time
// 
// 15    11/11/99 11:35a Jimdose
// added usetorso tp refEntity_t
// 
// 14    11/10/99 2:16p Jimdose
// added torso animation variables to refEntity_t
// 
// 13    10/29/99 6:52p Jimdose
// added bone controllers
// 
// 12    10/26/99 6:29p Jimdose
// added animation blending vars to refEntity_t
// 
// 11    10/25/99 12:19p Jimdose
// made head and torso angles local to each entity
// 
// 10    10/19/99 7:52p Markd
// Removed three part model system
// 
// 9     10/14/99 4:51p Jimdose
// added head_angles and torso_angles to refdef_t
// 
// 8     10/05/99 6:01p Aldie
// Added headers
//
// DESCRIPTION:
// Renderer types

#ifndef __TR_TYPES_H
#define __TR_TYPES_H


#define	MAX_DLIGHTS		32			// can't be increased, because bit flags are used on surfaces
#define	MAX_ENTITIES	1023		// can't be increased without changing drawsurf bit packing
#define  MAX_POINTS     32
#define  MAX_SPRITES    1024

// refdef flags
#define RDF_NOWORLDMODEL	1		// used for player configuration screen
#define RDF_HYPERSPACE		4		// teleportation effect


typedef struct {
	vec3_t		xyz;
	float		st[2];
	byte		modulate[4];
} polyVert_t;

typedef struct poly_s {
	qhandle_t			hShader;
	int					numVerts;
	polyVert_t			*verts;
} poly_t;


typedef enum {
   lensflare = ( 1 << 0 ),
   viewlensflare = ( 1 << 1 ),
   additive = ( 1 << 2 )
} dlighttype_t;

struct tikiFrame_s;
struct dtiki_s;


#define	MAX_RENDER_STRINGS			8
#define	MAX_RENDER_STRING_LENGTH	32


typedef enum {
	STEREO_CENTER,
	STEREO_LEFT,
	STEREO_RIGHT
} stereoFrame_t;


#if !defined _WIN32

#define _3DFX_DRIVER_NAME	"libMesaVoodooGL.so.3.1"
#define OPENGL_DRIVER_NAME	"libGL.so"

#else

#define _3DFX_DRIVER_NAME	"3dfxvgl"
#define OPENGL_DRIVER_NAME	"opengl32"

#endif	// !defined _WIN32


#endif	// __TR_TYPES_H
