#include "hooks.h"
#include "Menu.h"
#include "global.h"
#include "MaterialHelper.h"
#include "xor.h"
#include "BacktrackingHelper.h"
#include "Chams.h"
#include "Math.h"
template<class T, class U>
inline T clamp(T in, U low, U high)
{
	if (in <= low)
		return low;
	else if (in >= high)
		return high;
	else
		return in;
}

static IMaterial* CoveredLit;
static IMaterial* OpenLit;
static IMaterial* CoveredFlat;
static IMaterial* OpenFlat;
static IMaterial* backtrack;
static IMaterial* materialMetall = nullptr;
static IMaterial* materialMetallnZ = nullptr;

void Chams::CreateMaterialChams()
{
	CoveredLit = g_MaterialHelper->CreateMaterial(true);
	OpenLit = g_MaterialHelper->CreateMaterial(false);
	CoveredFlat = g_MaterialHelper->CreateMaterial(true, false);
	backtrack = g_MaterialHelper->CreateMaterial(true, false);
	OpenFlat = g_MaterialHelper->CreateMaterial(false, false);

	std::ofstream("csgo\\materials\\simple_ignorez_reflective.vmt") << R"#("VertexLitGeneric"
{

  "$basetexture" "vgui/white_additive"
  "$ignorez"      "1"
  "$envmap"       "env_cubemap"
  "$normalmapalphaenvmapmask"  "1"
  "$envmapcontrast"             "1"
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";

	std::ofstream("csgo\\materials\\simple_regular_reflective.vmt") << R"#("VertexLitGeneric"
{

  "$basetexture" "vgui/white_additive"
  "$ignorez"      "0"
  "$envmap"       "env_cubemap"
  "$normalmapalphaenvmapmask"  "1"
  "$envmapcontrast"             "1"
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";

	materialMetall = g_pMaterialSystem->FindMaterial("simple_ignorez_reflective", TEXTURE_GROUP_MODEL);
	materialMetallnZ = g_pMaterialSystem->FindMaterial("simple_regular_reflective", TEXTURE_GROUP_MODEL);
}

Chams::~Chams()
{
	std::remove("csgo\\materials\\simple_ignorez_reflective.vmt");
	std::remove("csgo\\materials\\simple_regular_reflective.vmt");
}

void __fastcall Hooks::scene_end(void* thisptr, void* edx) {

	static auto scene_end_o = renderviewVMT->GetOriginalMethod< decltype(&scene_end) >(9);
	scene_end_o(thisptr, edx);
	static bool init = false;
	if (!init)
	{
		Chams::get().CreateMaterialChams();
		init = true;
	}

	if (g_pEngine->IsConnected() && g_pEngine->IsInGame())
	{
		auto m_local = g::LocalPlayer;
		for (auto i = 0; i < g_GlowObjManager->size; i++)
		{
			if (!m_local)
				return;
			auto glow_object = &g_GlowObjManager->m_GlowObjectDefinitions[i];

			CBaseEntity *m_entity = glow_object->m_pEntity;

			if (!glow_object->m_pEntity || glow_object->IsUnused())
				continue;

			if (m_entity->IsPlayer())
			{
				if (Menu.Visuals.Glow && m_entity->GetTeamNum() != m_local->GetTeamNum())
				{
					float Red = Menu.Colors.Glow[0];
					float Green = Menu.Colors.Glow[1];
					float Blue = Menu.Colors.Glow[2];
					float A = Menu.Colors.Glow[3];

					glow_object->m_vGlowColor = Vector(Red, Green, Blue);
					glow_object->m_flGlowAlpha = A;

					glow_object->m_bRenderWhenOccluded = true;
					glow_object->m_bRenderWhenUnoccluded = false;
				}

				if (m_entity == m_local && Menu.Visuals.LGlow)
				{
					float Red = Menu.Colors.LGlow[0];
					float Green = Menu.Colors.LGlow[1];
					float Blue = Menu.Colors.LGlow[2];
					float A = Menu.Colors.LGlow[3];

					if (Menu.Visuals.PulseLGlow) {
						glow_object->m_bPulsatingChams = true;
					}
					glow_object->m_vGlowColor = Vector(Red, Green, Blue);
					glow_object->m_flGlowAlpha = A;

					glow_object->m_bRenderWhenOccluded = true;
					glow_object->m_bRenderWhenUnoccluded = false;
				}
			
			}
		}

		if (Menu.Visuals.ChamsEnable)
		{
			static IMaterial *covered;
			static IMaterial *open;

			switch (Menu.Visuals.ChamsStyle)
			{
			case 0:
				covered = CoveredLit;
				open = OpenLit;
				break;
			case 1:
				covered = CoveredFlat;
				open = OpenFlat;
				break;
			case 2:
				covered = materialMetall;
				open = materialMetallnZ;
				break;

			}

			for (int i = 1; i < 65; ++i) {
				CBaseEntity* ent = (CBaseEntity*)g_pEntitylist->GetClientEntity(i);

				if (ent == g::LocalPlayer && g::LocalPlayer != nullptr)
				{
					if (g::LocalPlayer->isAlive())
					{
						if (Menu.Visuals.ChamsL)
						{
							g_pRenderView->SetColorModulation(Menu.Colors.PlayerChamsl);

							g_pModelRender->ForcedMaterialOverride(open);
							g::LocalPlayer->draw_model(0x1, 255);
							g_pModelRender->ForcedMaterialOverride(nullptr);
						}
					}
				}

				if (ent->IsValidRenderable() && Menu.Visuals.ChamsPlayer)
				{
					if (Menu.Visuals.ChamsPlayerWall)
					{
						g_pRenderView->SetColorModulation(Menu.Colors.PlayerChamsWall);
						g_pModelRender->ForcedMaterialOverride(covered);
						ent->draw_model(0x1, 255);
						g_pModelRender->ForcedMaterialOverride(nullptr);
					}
					g_pRenderView->SetColorModulation(Menu.Colors.PlayerChams);
					g_pModelRender->ForcedMaterialOverride(open);
					ent->draw_model(0x1, 255);
					g_pModelRender->ForcedMaterialOverride(nullptr);
				}

				if (ent->IsValidRenderable() && Menu.Visuals.ShowBacktrack)
				{
					Vector orig = ent->GetAbsOrigin();

					if (g_BacktrackHelper->PlayerRecord[i].records.size() > 0)
					{
						tick_record record = g_BacktrackHelper->PlayerRecord[i].records.at(0);

						if (orig != record.m_vecAbsOrigin)
						{
							g_pRenderView->SetColorModulation(Menu.Colors.ChamsHistory);
							g_pRenderView->SetBlend(0.7f);
							ent->SetAbsOrigin(record.m_vecAbsOrigin);
							ent->UpdateClientSideAnimation();
							g_pModelRender->ForcedMaterialOverride(CoveredFlat);
							ent->draw_model(0x1, 255);
							g_pModelRender->ForcedMaterialOverride(nullptr);

							ent->SetAbsOrigin(orig);
						}
					}
				}
			}
		}
	}

	
}

void AngleMatrix(const Vector angles, matrix3x4_t& matrix)
{
	float sr, sp, sy, cr, cp, cy;

	sy = sin(DEG2RAD(angles[1]));
	cy = cos(DEG2RAD(angles[1]));

	sp = sin(DEG2RAD(angles[0]));
	cp = cos(DEG2RAD(angles[0]));

	sr = sin(DEG2RAD(angles[2]));
	cr = cos(DEG2RAD(angles[2]));

	//matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp * cy;
	matrix[1][0] = cp * sy;
	matrix[2][0] = -sp;

	float crcy = cr * cy;
	float crsy = cr * sy;
	float srcy = sr * cy;
	float srsy = sr * sy;

	matrix[0][1] = sp * srcy - crsy;
	matrix[1][1] = sp * srsy + crcy;
	matrix[2][1] = sr * cp;

	matrix[0][2] = (sp * crcy + srsy);
	matrix[1][2] = (sp * crsy - srcy);
	matrix[2][2] = cr * cp;

	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}

void MatrixSetColumn(const Vector &in, int column, matrix3x4_t& out)
{
	out[0][column] = in.x;
	out[1][column] = in.y;
	out[2][column] = in.z;
}

void AngleMatrix(const QAngle &angles, const Vector &position, matrix3x4_t& matrix_out)
{
	AngleMatrix(angles, matrix_out);
	MatrixSetColumn(position, 3, matrix_out);
}

void MatrixCopy(const matrix3x4_t& source, matrix3x4_t& target)
{
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) {
			target[i][j] = source[i][j];
		}
	}
}

void MatrixMultiply(matrix3x4_t& in1, const matrix3x4_t& in2)
{
	matrix3x4_t out;
	if (&in1 == &out)
	{
		matrix3x4_t in1b;
		MatrixCopy(in1, in1b);
		MatrixMultiply(in1b, in2);
		return;
	}
	if (&in2 == &out)
	{
		matrix3x4_t in2b;
		MatrixCopy(in2, in2b);
		MatrixMultiply(in1, in2b);
		return;
	}
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
		in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
		in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
		in1[2][2] * in2[2][3] + in1[2][3];

	in1 = out;
}

void VectorRotate(const float *in1, const matrix3x4_t& in2, float *out)
{
	out[0] = DotProduct(in1, in2[0]);
	out[1] = DotProduct(in1, in2[1]);
	out[2] = DotProduct(in1, in2[2]);
}

void VectorRotate(const Vector& in1, const matrix3x4_t &in2, Vector &out)
{
	VectorRotate(&in1.x, in2, &out.x);
}

void VectorRotate(const Vector &in1, const QAngle &in2, Vector &out)
{
	matrix3x4_t matRotate;
	AngleMatrix(in2, matRotate);
	VectorRotate(in1, matRotate, out);
}

void __fastcall Hooks::DrawModelExecute(void* ecx, void* edx, void* * ctx, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld)
{

	if (!g::LocalPlayer)
	{
		modelrenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(ecx, ctx, state, pInfo, pCustomBoneToWorld);
		return;
	}

	auto ent = g_pEntitylist->GetClientEntity(pInfo.entity_index);

	const char* ModelName = g_pModelInfo->GetModelName((model_t*)pInfo.pModel);


	if (ent == g::LocalPlayer)
	{
		if (Menu.Visuals.FakeChams && (g::LocalPlayer->GetBasePlayerAnimState() != nullptr)) {
			Vector BonePos;
			Vector OutPos;
			matrix3x4_t BoneMatrix[MAXSTUDIOBONES];
			for (int i = 0; i < MAXSTUDIOBONES; i++)
			{
				AngleMatrix(Vector(0, Math::NormalizeYaw(g::LocalPlayer->GetBasePlayerAnimState()->m_flGoalFeetYaw), 0), BoneMatrix[i]);
				MatrixMultiply(BoneMatrix[i], pCustomBoneToWorld[i]);
				BonePos = Vector(pCustomBoneToWorld[i][0][3], pCustomBoneToWorld[i][1][3], pCustomBoneToWorld[i][2][3]) - pInfo.origin;
				VectorRotate(BonePos, Vector(0, g::FakeAngle.y, 0), OutPos);
				OutPos += g::fakeOrigin;

				BoneMatrix[i][0][3] = OutPos.x;
				BoneMatrix[i][1][3] = OutPos.y;
				BoneMatrix[i][2][3] = OutPos.z;

			}
			float clr[3] = { 0.f, 0.7f, 0.7f };
			
			static auto cur_material = g_pMaterialSystem->FindMaterial("models/inventory_items/dogtags/dogtags_outline", TEXTURE_GROUP_OTHER);
			cur_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);
			g_pModelRender->ForcedMaterialOverride(cur_material);
			g_pRenderView->SetColorModulation(clr);
			modelrenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(ecx, ctx, state, pInfo, BoneMatrix);
		}
		g_pRenderView->SetBlend(Menu.Visuals.playeralpha / 255.f);
		if (Menu.Visuals.ChamsL)
		{
			static IMaterial *covered;
			static IMaterial *open;

			switch (Menu.Visuals.ChamsStyle)
			{
			case 0:
				covered = CoveredLit;
				open = OpenLit;
				break;
			case 1:
				covered = CoveredFlat;
				open = OpenFlat;
				break;
			case 2:
				covered = materialMetall;
				open = materialMetallnZ;
				break;

			}

			g_pRenderView->SetColorModulation(Menu.Colors.PlayerChamsl);

			g_pModelRender->ForcedMaterialOverride(open);
			modelrenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(ecx, ctx, state, pInfo, pCustomBoneToWorld);
		}
		else {
			float clr2[3] = { 1, 1, 1 };
			g_pRenderView->SetColorModulation(clr2);
			g_pModelRender->ForcedMaterialOverride(nullptr);
			modelrenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(ecx, ctx, state, pInfo, pCustomBoneToWorld);
		}
	}
	else if (ent->IsValidRenderable() && strstr(ModelName, "models/player")) {
		g_pRenderView->SetBlend(Menu.Visuals.entplayeralpha / 255.f);
	}
	const auto mdl = pInfo.pModel;

	if (Menu.Colors.Props) {
		if (strstr(ModelName, "models/props")) {
			g_pRenderView->SetBlend(0.5f);
		}
	}

	if (Menu.Misc.WireHand) {
		if (strstr(ModelName, "arms")) {
			static IMaterial* mat = g_MaterialHelper->CreateMaterial(true, false, true);
			mat->ColorModulate(Menu.Colors.styleshands[0] * 255, Menu.Colors.styleshands[1] * 255, Menu.Colors.styleshands[2] * 255);
			g_pModelRender->ForcedMaterialOverride(mat);
		}
	}

	static IMaterial *xblur_mat = g_pMaterialSystem->FindMaterial("dev/blurfilterx_nohdr", TEXTURE_GROUP_OTHER, true);
	static IMaterial *yblur_mat = g_pMaterialSystem->FindMaterial("dev/blurfiltery_nohdr", TEXTURE_GROUP_OTHER, true);
	static IMaterial *scope = g_pMaterialSystem->FindMaterial("dev/scope_bluroverlay", TEXTURE_GROUP_OTHER, true);
	xblur_mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
	yblur_mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
	scope->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);

	modelrenderVMT->GetOriginalMethod<DrawModelExecuteFn>(21)(ecx, ctx, state, pInfo, pCustomBoneToWorld);
	g_pModelRender->ForcedMaterialOverride(NULL);
}
