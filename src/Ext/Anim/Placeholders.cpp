
//DEFINE_HOOK(0x423AC0, AnimClass_Update, 0x6)
//{
	//GET(AnimClass*, pThis, ECX);
	//auto pExt = AnimExt::ExtMap.Find(pThis);

	//if (!pExt && pThis->Type)
	//	Debug::Log("Failed ! To Find Ext for [%s] ! \n", pThis->get_ID());

	//return 0x0;
//}

//crash and corrup ESI pointer around
//DEFINE_HOOK(0x424F00, AnimClass_Middle_Broken, 0x4)
//{
	//GET(AnimClass*, pThis, ECX);
	//auto pExt = AnimExt::ExtMap.Find(pThis);
//	return 0x0;
//}

/* Original Ares
int __cdecl AnimClass_Expired_SpawnsParticle(REGISTERS *a1)
{
  signed int v1; // esi
  unsigned int v2; // ebx
  int v3; // eax
  int v4; // ecx
  int result; // eax
  ScenarioClass *v6; // edi
  int v7; // eax
  int v8; // ebx
  double v9; // st7
  double v10; // [esp+50h] [ebp-50h]
  int v11; // [esp+58h] [ebp-48h]
  int v12; // [esp+5Ch] [ebp-44h]
  int v13; // [esp+60h] [ebp-40h]
  double v14; // [esp+60h] [ebp-40h]
  ParticleTypeClass *pType; // [esp+6Ch] [ebp-34h]
  double v16; // [esp+70h] [ebp-30h]
  double v17; // [esp+78h] [ebp-28h]
  double v18; // [esp+80h] [ebp-20h]
  CoordStruct coord_4; // [esp+88h] [ebp-18h] BYREF
  CoordStruct v20; // [esp+94h] [ebp-Ch] BYREF

  v1 = a1->_ECX.data;
  v2 = a1->_ESI.data;
  pType = MEMORY[0xA83D9C][*(a1->_EAX.data + 716)];
  v3 = GetExt(AnimTypeExt, a1->_EAX.data);
  v4 = *(v3 + 24);
  result = *(v3 + 28);
  v12 = v4;
  v11 = result;
  if ( v4 || result )
  {
	v6 = MEMORY[0xA8B230];
	(*(*v2 + 72))(v2, &v20);
	v7 = CellClass::GetCellFloorHeight(0x87F7E8, &v20);
	v8 = v20.Z - v7;
	v17 = 6.283185307179586 / v1;
	v16 = 0.0;
	if ( v1 > 0 )
	{
	  do
	  {
		v13 = abs32(Randomizer::RandomRanged(&v6->Random, v12, v11));
		v10 = Randomizer::RandomRanged(&v6->Random, 1, 0x7FFFFFFF) * 4.656612873077393e-10 * v17 + v16;
		v18 = FastMath::Cos(v10);
		v9 = FastMath::Sin(v10);
		coord_4.Z = v20.Z;
		v14 = v13;
		coord_4.X = v20.X + (v14 * v18);
		coord_4.Y = v20.Y - (v9 * v14);
		coord_4.Z = v8 + CellClass::GetCellFloorHeight(8910824, &coord_4);
		ParticleSystemClass::SpawnParticle(MEMORY[0xA8ED78], pType, &coord_4);
		v16 = v16 + v17;
		--v1;
	  }
	  while ( v1 );
	}
	result = 0x42504D;
  }
  return result;
}
*/