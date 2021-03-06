#include "sdk.h"
#include "Singleton.h"

struct animfix_data {
	CAnimationLayer layer[15];
	float* pose_parameter;
	float old_simtime;
	float old_fraction;
	std::array<float, 24> old_ragpos;
	CBasePlayerAnimState* old_animstate;
}animfix[65];

class c_animfix : public Singleton<c_animfix> {
public:
	void on_fsn(ClientFrameStage_t stage) {
		if (stage != FRAME_RENDER_START)
			return;
		for (int i = 1; i < 65; ++i) {
			auto entity = g_pEntitylist->GetClientEntity(i);
			if (!entity || !g::LocalPlayer || !entity->isAlive())
				continue;

			if (animfix[i].old_simtime != entity->GetSimulationTime()) {
			
				entity->GetClientSideAnimation() = true;
				entity->UpdateClientSideAnimation();

				if (entity->GetBasePlayerAnimState())
					entity->GetBasePlayerAnimState()->m_iLastClientSideAnimationUpdateFramecount = g_pGlobals->framecount - 1;

				memcpy(animfix[i].layer, entity->GetAnimOverlays(), (sizeof(AnimationLayer) * 15));
				animfix[i].old_ragpos = entity->get_ragdoll_pos();
				animfix[i].old_animstate = entity->GetBasePlayerAnimState();

				//update lol
				animfix[i].old_simtime = entity->GetSimulationTime();
			}
			entity->GetClientSideAnimation() = false;

			memcpy(entity->GetAnimOverlays(), animfix[i].layer, (sizeof(AnimationLayer) * 15));
			entity->get_ragdoll_pos() = animfix[i].old_ragpos;
			*(CBasePlayerAnimState**)((DWORD)entity + 0x3900) = animfix[i].old_animstate;
			
			g::LocalPlayer->SetAngle2(Vector(0, g::LocalPlayer->GetBasePlayerAnimState()->m_flGoalFeetYaw, 0));
		}
	}
};