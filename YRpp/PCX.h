//This can be used to load PCX files into BSurfaces!

#pragma once

#include <GeneralDefinitions.h>
#include <Helpers/CompileTime.h>
#include <RectangleStruct.h>

struct BytePalette;
class BSurface;
class DSurface;
class PCX
{
public:

	static COMPILETIMEEVAL reference<PCX, 0xAC4848u> const Instance{};
	static OPTIONALINLINE COMPILETIMEEVAL WORD const DefaultTransparentColor = COLOR_PURPLE;

	// Load a PCX file
	bool ForceLoadFile(const char* pFileName, int flag1, int flag2)
		{ JMP_THIS(0x6B9D00); }

	//Load a PCX file
	bool LoadFile(const char *pFileName, int flag1 = 2, int flag2 = 0);

	// Get a BSurface for a PCX file. File needs to be loaded some time first!
	BSurface* GetSurface(const char* pFileName, BytePalette * pPalette = nullptr)
		{ JMP_THIS(0x6BA140); }

	// Draws a PCX file
	bool BlitToSurface(RectangleStruct *BoundingRect,
		DSurface* TargetSurface,
		BSurface* PCXSurface,
		WORD TransparentColor = DefaultTransparentColor
	)
		{ JMP_THIS(0x6BA580); }

	void *Buffer;
};
