#define g_pGameFramework			0xB0E04C
#define g_pViewer					(g_pGameFramework+0x4)
#define g_pPlayer					(g_pGameFramework+0xC) 
#define g_pCharaManager 			(g_pGameFramework+0x10) 
#define g_pWeaponManager 			(g_pGameFramework+0x14)
#define g_pWeaponEdit 				(g_pGameFramework+0x28) 
#define g_pGameControl 				(g_pGameFramework+0x3C) 
#define g_pGameContext 				(g_pGameFramework+0x44)

class i3Viewer;
class i3RenderContext;
class Player;
class PlayerName;
class PlayerHealth;
class i3Camera;
class i3SceneTracer;
class i3SoundContext;
class i3InputDeviceManager;
class i3Framework;
class i3AttrStackManager;
class i3RenderAttrList;
class i3MatrixStack;
class i3SceneGraphContext;
class i3StageInfo;
class i3Stage;
class i3World;
class i3GameObj;
class i3SceneObject;
class i3ViewMatrixAttr;
class i3InputMouse;
class i3GameSceneInfo;
class CGameCharaBone;
class CGameCharaBoneContext;
class Bone;
class CHARA_DAMAGEINFO;
class i3PhysixHitResult;
class i3CollideeLine;
class CGameMissionManager;
class CDeathPlayer;
//-----------------------------------// 
class CDeathPlayer
{
public:
	BYTE Death[16];
};
class i3Viewer
{
public: char _0x0000[8];
		DWORD State; //0x0008 
		void* i3PhysixContext; //0x000C 
		i3RenderContext* RenderContext; //0x0010 
		i3SceneTracer* SceneTracer; //0x0014 
		i3SoundContext* SoundContext; //0x0018 
		char _0x001C[8];
		i3InputDeviceManager* InputDeviceManager; //0x0024 
		void* Timer; //0x0028 
		float UserTime; //0x002C 
		float TimeScale; //0x0030 
		float DeltaTime; //0x0034 
		i3Framework* Framework; //0x0038 
		HWND WindowHandle; //0x003C 
		char _0x0040[16];
		BYTE IsFixedResolution; //0x0050 
		char _0x0051[3];
public:
	static i3Viewer* Singleton()
	{
		return (i3Viewer*)(*(DWORD*)(g_pViewer));
	}
};//Size=0x0054 
class i3RenderContext
{
public: char _0x0000[36];
		DWORD IsWindowed; //0x0024 
		DWORD IsReady; //0x0028 
		char _0x002C[124];
		DWORD EndScene; //0x00A8 
		char _0x00AC[2068];
		D3DXMATRIX ViewMatrix; //0x08C0 
		D3DXMATRIX ProjMatrix; //0x900 
		D3DXMATRIX WorldMatrix;//0x940 
		char unk0x980[18716];
		DWORD WindowWidth;//0x529C 
		DWORD WindowHeight;//0x52A0 
		char unk0x52A4[192];
		IDirect3D9* D3D9; //0x5364 
		LPDIRECT3DDEVICE9 pDevice; //0x5368 
		char unk0x536C[20];
		HWND hWindow; //0x5380 
};
class Player
{
public:
	float Yaw;
	float Pitch;
	D3DXVECTOR3 Pos;
};
class PlayerName
{
public:
	char Name[33];
};
class PlayerHealth
{
public:
	float Health; //0x0000 
	char unk[28];
};//Size=0x0020; 
class i3Camera
{
public:
	char _0x0000[144];
	float FOV; //0x0090 
	float Near; //0x0094 
	float Far; //0x0098 
	DWORD WindowWidth; //0x009C 
	DWORD WindowHeight; //0x00A0 
	DWORD Mode; //0x00A4 
	DWORD Enable; //0x00A8 
	DWORD Style; //0x00AC 
	float UpdateInterval; //0x00B0 
	void* ProjMatrixAttr; //0x00B4 
	i3ViewMatrixAttr* ViewMatrixAttr; //0x00B8 
	void* RenderTargetAttr; //0x00BC 
	void* ClearRenderTargetAttr; //0x00C0 
	void* OutputList; //0x00C4 
	void* SubCamSg; //0x00C8 
};//Size=0x00CC 
class i3SceneTracer
{
public:
	char _0x0000[8];
	DWORD Style; //0x0008 
	DWORD State; //0x000C 
	i3AttrStackManager* AttrStackManager; //0x0010 
	i3RenderContext* RenderContext; //0x0014 
	i3RenderAttrList* Output; //0x0018 
	char _0x001C[4];
	i3MatrixStack* ModelViewMatrixStack; //0x0020 
	char _0x0024[4];
	void* CollisionContext; //0x0028 i3CollisonContext 
	i3SceneGraphContext* SgContext; //0x002C 
	i3SoundContext* SoundContext; //0x0030 
	void* PhysiXContext; //0x0034 
	DWORD DisableNodeMask; //0x0038 
	char _0x003C[12];
	void* MatrixEnvironment; //0x0048 _tagMatrixEnvironmentInfo 
	char _0x004C[48];
	float Time; //0x007C 
	i3Camera* CurrentCamera; //0x0080 
	char _0x0084[12];
	D3DXMATRIX ViewMatrix; //0x0090 
	D3DXMATRIX InverseViewMatrix; //0x00D0 
	D3DXMATRIX ProjectionMatrix; //0x0110 
	D3DXMATRIX ViewProjectionMatrix; //0x0150 
	DWORD FrustumCullEnable; //0x0190 
	char _0x0194[192];
	DWORD PlaneMask; //0x0254 
	DWORD FrameID; //0x0258 
	DWORD CulledState; //0x025C 
};
class i3SoundContext
{
public:
	char _0x0000[80];
	D3DXMATRIX ListnerMatrix; //0x0050 
	char _0x0054[12];
	BYTE N003E3217; //0x009C 
	BYTE NeedUpdateListener; //0x009D 
	char _0x009E[2]; DWORD State; //0x00A0 
	char _0x00A4[16]; BYTE MainVolume; //0x00B4 
	char _0x00B5[7]; BYTE PendindEnable; //0x00BC 
	char _0x00BD[3];
};
class i3InputDeviceManager
{
public:
	char _0x0000[8];
	void* InputKeyboard; //0x0008 i3InputKeyboard * 
	i3InputMouse* InputMouse; //0x000C i3InputMouse 
	char _0x0010[32];
	DWORD IsMapped; //0x0030 
	char _0x0034[8];
	DWORD AnalogMapCount; //0x003C 
	DWORD GetButtonMapCount; //0x0040 
};//Size=0x0044 
class i3Framework
{
public:
	char _0x0000[96];
	i3StageInfo* StartStageInfo; //0x0060 i3StageInfo 
	void* CmnResList; //0x0064 i3StringList 
	char _0x0068[8];
	DWORD State; //0x0070 
	char _0x0074[20];
	float LocalTime; //0x0088 
	i3Viewer* Viewer; //0x008C 
	void* ResourceManager; //0x0090 i3ResourceManager 
	void* EffectManager; //0x0094 i3EffectManager 
	i3Stage* CurrentStage; //0x0098 i3Stage 
	char _0x009C[24];
	DWORD PrevStageIndex; //0x00B4 
	i3SceneGraphContext* SgContext; //0x00B8 i3SceneGraphContext 
	DWORD FadeEffect; //0x00BC 
	float StageSwitchFadeTime; //0x00C0 
	i3Camera* DefaultCamera; //0x00C4 i3Camera 
	char _0x00C8[28];
	void* OnMouseTarget; //0x00E4 i3EventReceiver 
	void* InputDispatchManager; //0x00E8 i3InputDispatchManager 
	void* UIManager; //0x00EC i3UIManager 
	char _0x00F0[4];
	void* UIRenderer; //0x00F4 i3UIRenderer 
};//Size=0x00F8 
class i3AttrStackManager
{
public:
	char _0x0000[8];
	BYTE IsEnable; //0x0008 
};
class i3RenderAttrList
{
public:
	char _0x0000[16];
	void* AttrList; //0x0010 i3List 
	char _0x0014[8];
	float Dist; //0x001C 
};//Size=0x0020 
class i3MatrixStack
{
public:
	char _0x0000[8];
	DWORD IsModified; //0x0008 
};//Size=0x000C 
class i3SceneGraphContext
{
public:
	char _0x0000[8];
	void* LightList; //0x0008 i3List 
	char _0x000C[12];
	void* _3DRoot; //0x0018 i3LayerRootNode 
	void* _2DRoot; //0x001C i3AttrSet 
	void* DebugTextRoot; //0x0020 i3AttrSet 
	char _0x0024[20];
	BYTE BoundEnable; //0x0038 
	char _0x0039[107];
	void* CurrentCamera; //0x00A4 i3Camera 
	char _0x00A8[4];
	DWORD CurrentPortal; //0x00AC 
};//Size=0x00B0 
class i3StageInfo
{
public:
	char _0x0000[20];
	DWORD FadeInColor; //0x0014 
	float FadeInTime; //0x0018 
	DWORD FadeOutColor; //0x001C 
	float FadeOutTime; //0x0020 
	void* NextStage; //0x0024 i3StageInfo 
	DWORD JumpType; //0x0028 
	void* RSCPath; //0x002C i3StringList 
	char _0x0030[8];
	void* InstanceManagerInfoList; //0x0038 i3List 
	char _0x003C[4];
	void* ElementInstanceManagerInfoList; //0x0040 i3List 
};//Size=0x0044 
class i3Stage
{
public:
	char _0x0000[72];
	i3Framework* Framework; //0x0048 i3Framework 
	i3World* World; //0x004C i3World 
	i3GameSceneInfo* GameSceneInfo; //0x0050 i3GameSceneInfo 
	void* RespawnObjList; //0x0054 i3List 
	char _0x0058[36];
	DWORD LayerCount; //0x007C 
	char _0x0080[308];
	float BattleIdle; //0x01B4 
	float RoomIdle; //0x01B8 
};//Size=0x01BC 
class i3World
{
public:
	char _0x0000[240];
	void* WorldRoot; //0x00F0 i3AttrSet 
	void* SkyRoot; //0x00F4 i3AttrSet 
	char _0x00F8[44];
	void* SgWorld; //0x0124 i3SceneGraphInfo 
	void* SgSky; //0x0128 i3SceneGraphInfo 
	void* SgWorldCollision; //0x012C i3SceneGraphInfo 
	void* SgHitCollition; //0x0130 i3SceneGraphInfo 
	void* SgLightVolumeCollision; //0x0134 i3SceneGraphInfo 
	void* WorldSectionTable; //0x0138 i3WorldSectionTable 
	void* MapSectorTable; //0x013C i3MapSectorTable 
};//Size=0x0140 
class i3AttrList
{
public:
	char _0x0000[24];
	void* DefaultAttr; //0x0018 i3RenderAttr 
	char _0x001C[4]; DWORD TraceMode; //0x0020 
	DWORD ID; //0x0024 
	void* Next; //0x0028 i3AttrList 
	char _0x002C[8];
	void* Attrs; //0x0034 
	char _0x0038[8];
	DWORD Priority; //0x0040
};//Size=0x0044 
class i3GameObj
{
public:
	char _0x0000[100];
	void* AttachNode; //0x0064 i3Node 
	char _0x0068[24];
	D3DXMATRIX InitialMatrix; //0x0080 
	char _0x00C0[8];
	DWORD ActiveState; //0x00C8 
	BYTE DynamicActiveState; //0x00CC 
	char _0x00CD[3];
	i3SceneObject* SceneObject; //0x00D0 i3SceneObject 
	char _0x00D4[4];
	DWORD PortalID; //0x00D8 
	char _0x00DC[12];
	DWORD Layer; //0x00E8 
};//Size=0x00EC 
class i3SceneObject
{
public:
	char _0x0000[144];
	float TimeScale; //0x0090 
	void* BoneList; //0x0094 i3BinList 
	char _0x0098[4];
	void* Body; //0x009C i3Body 
	void* WorldSpaceSource; //0x00A0 i3Node 
	void* CurrentAnim; //0x00A4 i3Animation 
};//Size=0x00A8 
class i3ViewMatrixAttr
{
public:
	char _0x0000[16];
	D3DXMATRIX Matrix; //0x0010 
};//Size=0x0050 
class i3InputMouse
{
public:
	char _0x0000[16];
	float GetX; //0x0010 
	float GetY; //0x0014 
	float GetZ; //0x0018
	DWORD ButtonState; //0x001C
	DWORD StrokeButtonState; //0x0020 
	float DeltaX; //0x0024
	float DeltaY; //0x0028 
	float DeltaZ; //0x002C
};//Size=0x0030 
class i3GameSceneInfo
{
public:
	char _0x0000[20];
	DWORD InstanceCount; //0x0014 
};//Size=0x0018 
class CGameCharaBase
{
public:
	char _0x0000[8];
	void* pRes; //0x0008 
	void* pCharaInfo; //0x000C 
	char _0x0010[284];
	void* m_pCharaInfo; //0x012C 
	char _0x0130[4];
	void* m_pUpdateContext; //0x0134 
	void* m_pInputContext; //0x0138 
	void* m_pActionContext; //0x013C 
	void* m_pAnimContext; //0x0140 
	void* m_pWeaponContext; //0x0144 
	void* m_pMoveContext; //0x0148 
	char _0x014C[4];
	void* m_pEquipContext; //0x0150 
	void* m_pCameraContext; //0x0154
	CGameCharaBoneContext* m_BoneContext; //0x0158
	void* m_pSoundContext; //0x015C 
	void* m_pCollisionContext; //0x0160
	void* m_pAIContext; //0x0164
	void* m_pHUDContext; //0x0168
};//Size=0x016C 

class CGameCharaBoneContext
{
public:
	char _0x0000[228];
	int* m_IdxBone; //0x00E4 
	Bone* m_Bone; //0x00E8 
};//Size=0x00EC 
class Bone
{
public:
	D3DXMATRIX m_Bone;
};
class CHARA_DAMAGEINFO
{
public:
	char _0x0000[20];
	DWORD nKillerIdxz; //0x0014 
	char _0x0018[4];
	DWORD nVictimIdx; //0x001C 
	DWORD WeaponSlot; //0x0020 
	DWORD WeaponNumber; //0x0024 
	char _0x0028[16];
	DWORD HitPart; //0x0038 
	char _0x003C[4];
};//Size=0x0040 
class i3PhysixHitResult
{
public:
	char _0x0000[28];
	D3DXVECTOR3 FinalPos; //0x001C 
	float Distance; //0x0028 
};//Size=0x0028 
class i3CollideeLine
{
public:
	char _0x0000[12];
	D3DXVECTOR3 vStart; //0x000C 
	D3DXVECTOR3 vEnd; //0x0018 
	D3DXVECTOR3 vDir; //0x0024 
};//Size=0x0030 
class CGameMissionManager
{
public:
	char _0x0000[560];
	D3DXVECTOR3 BombA; //0x0230 
	D3DXVECTOR3 BombB; //0x023C
	char _0x0248[64];
};