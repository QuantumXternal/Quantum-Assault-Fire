#include <d3d9.h>
#include <d3dx9.h>
#include "Quelle.h"
#include "color.h"
#include "MinHook.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx9.h"
#include "int.h"
#include "Hacks.h"
#include "Class.h"
#include "d3dmenu.h"
#include <iostream>
#include <stdio.h>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <process.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "D3DX9.LIB")
#pragma comment(lib, "ntdll.lib")
#define ThreadHideFromDebugger 0x11
typedef LONG NTSTATUS_RAW;
extern "C" NTSTATUS_RAW NTAPI NtSetInformationThread(HANDLE ThreadHandle, ULONG ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);

static void HideCurrentThreadFromDebugger() {
	NtSetInformationThread(GetCurrentThread(), ThreadHideFromDebugger, nullptr, 0);
}

static const unsigned char kWinmmEncoded[] = { 0x2d, 0x33, 0x34, 0x37, 0x37, 0x74, 0x3e, 0x36, 0x36 };
static const unsigned char kModuleXorKey = 0x5A;

static void XorDecryptString(const unsigned char* encoded, size_t encodedLength, char* output, size_t outputSize, unsigned char key) {
	if (!encoded || !output || outputSize == 0) {
		return;
	}

	const size_t copyLength = (encodedLength < outputSize - 1) ? encodedLength : outputSize - 1;
	for (size_t i = 0; i < copyLength; ++i) {
		output[i] = static_cast<char>(encoded[i] ^ key);
	}
	output[copyLength] = '\0';
}

static void DecodeModuleName(char* output, size_t outputSize) {
	XorDecryptString(kWinmmEncoded, sizeof(kWinmmEncoded), output, outputSize, kModuleXorKey);
}

static const char kTitle[] = { 'C','h','e','a','t',' ','N','i',' ','M','o','n','g',' ','-',' ','J','\0' };
static const char kHeader[] = { 'C','H','E','A','T',' ','N','I',' ','M','O','N','G',' ','-',' ','J','\0' };


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
static WNDPROC oWndProc = nullptr;
bool bEnableChams = false;
bool bMenuOpen = true;
bool bEnableCrosshair = false;
bool bShowWatermark = true;
float crosshairSize = 10.0f;
float crosshairThickness = 2.0f;
float crosshairGap = 4.0f;
float crosshairColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
float chamsColor[4] = { 1.0f, 0.05f, 0.05f, 1.0f };
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

i3RenderContext *rContext;

bool Color = true;
bool Create;

LPDIRECT3DVERTEXBUFFER9 StreamData;
UINT OffsetInBytes;
UINT m_Stride;
bool g_IsRenderingImGui = false;
LPDIRECT3DTEXTURE9 Red,Green,Blue;
D3DVIEWPORT9    g_ViewPort;
float ScreenCenterX = 0.0f; 
float ScreenCenterY = 0.0f; 


static bool IsAddressReadable(uintptr_t address, size_t size) {
	if (address == 0 || size == 0 || address + size < address) return false;

	MEMORY_BASIC_INFORMATION mbi = {};
	if (VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) != sizeof(mbi)) return false;
	if (mbi.State != MEM_COMMIT) return false;

	DWORD protect = mbi.Protect;
	if ((protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0) return false;

	const DWORD readableMask = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
		PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
	if ((protect & readableMask) == 0) return false;

	uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
	uintptr_t regionEnd = regionStart + mbi.RegionSize;
	return address >= regionStart && address + size <= regionEnd;
}

static bool ReadUInt32(uintptr_t address, uintptr_t* outValue) {
	if (!outValue || !IsAddressReadable(address, sizeof(uintptr_t))) return false;
	std::memcpy(outValue, reinterpret_cast<void*>(address), sizeof(uintptr_t));
	return true;
}


static bool TryGetGameWindow(HWND* outWindow) {
	if (!outWindow) return false;

	uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
	uintptr_t gameFramework = 0;
	uintptr_t viewer = 0;
	uintptr_t windowHandle = 0;

	if (!ReadUInt32(base + 0xB0E04C, &gameFramework) || !gameFramework) return false;
	if (!ReadUInt32(base + 0xB0E04C + 0x4, &viewer) || !viewer) return false;
	if (!ReadUInt32(viewer + 0x3C, &windowHandle) || !windowHandle) return false;

	HWND window = reinterpret_cast<HWND>(windowHandle);
	if (!IsWindow(window)) return false;

	*outWindow = window;
	return true;
}

typedef struct _UNICODE_STRING32 {
	USHORT Length;
	USHORT MaximumLength;
	WCHAR* Buffer;
} UNICODE_STRING32;

static void UnlinkListEntry(LIST_ENTRY* entry) {
	if (!entry || !entry->Flink || !entry->Blink) {
		return;
	}

	entry->Blink->Flink = entry->Flink;
	entry->Flink->Blink = entry->Blink;
	entry->Flink = nullptr;
	entry->Blink = nullptr;
}

static void ZeroUnicodeString(UNICODE_STRING32* string) {
	if (!string) {
		return;
	}

	if (string->Buffer && string->Length) {
		RtlZeroMemory(string->Buffer, string->Length);
	}

	string->Length = 0;
	string->MaximumLength = 0;
	string->Buffer = nullptr;
}

void UnlinkFromPEB(HMODULE hModule) {
	PVOID peb = nullptr;
	__asm {
		mov eax, fs:[0x30]
		mov peb, eax
	}

	if (!peb) {
		return;
	}

	BYTE* pebBytes = reinterpret_cast<BYTE*>(peb);
	BYTE* loaderData = *reinterpret_cast<BYTE**>(pebBytes + 0x0C);
	if (!loaderData) {
		return;
	}

	LIST_ENTRY* loadOrderHead = reinterpret_cast<LIST_ENTRY*>(loaderData + 0x0C);
	LIST_ENTRY* current = loadOrderHead->Flink;

	while (current && current != loadOrderHead) {
		BYTE* entry = reinterpret_cast<BYTE*>(current);
		HMODULE entryBase = *reinterpret_cast<HMODULE*>(entry + 0x18);

		if (entryBase == hModule) {
			UnlinkListEntry(reinterpret_cast<LIST_ENTRY*>(entry + 0x00));
			UnlinkListEntry(reinterpret_cast<LIST_ENTRY*>(entry + 0x08));
			UnlinkListEntry(reinterpret_cast<LIST_ENTRY*>(entry + 0x10));
			ZeroUnicodeString(reinterpret_cast<UNICODE_STRING32*>(entry + 0x24));
			ZeroUnicodeString(reinterpret_cast<UNICODE_STRING32*>(entry + 0x2C));
			break;
		}

		current = reinterpret_cast<LIST_ENTRY*>(current)->Flink;
	}

}

typedef HRESULT ( WINAPI* oEndScene ) ( LPDIRECT3DDEVICE9 pDevice);
oEndScene pEndScene;

bool WorldToScreen(float* matrix, float* in, float* out, float width, float height) {
    if (!matrix || !in || !out) return false;

    float x = in[0] * matrix[0] + in[1] * matrix[4] + in[2] * matrix[8] + matrix[12];
    float y = in[0] * matrix[1] + in[1] * matrix[5] + in[2] * matrix[9] + matrix[13];
    float w = in[0] * matrix[3] + in[1] * matrix[7] + in[2] * matrix[11] + matrix[15];

    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) || w <= 0.01f) return false;

    float invw = 1.0f / w;
    float ndcX = x * invw;
    float ndcY = y * invw;

    if (!std::isfinite(ndcX) || !std::isfinite(ndcY)) return false;
    if (width <= 0.0f || height <= 0.0f) return false;

    out[0] = (width / 2.0f) + (ndcX * width / 2.0f);
    out[1] = (height / 2.0f) - (ndcY * height / 2.0f);
    return std::isfinite(out[0]) && std::isfinite(out[1]);
}

static DWORD WINAPI UnloadDllThread(LPVOID) {
	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
	Sleep(200);

	char moduleName[16];
	DecodeModuleName(moduleName, sizeof(moduleName));
	HMODULE module = GetModuleHandle(moduleName);
	if (module) {
		FreeLibraryAndExitThread(module, 0);
	}
	return 0;
}

static bool CustomToggle(const char* label, bool* value) {
	ImGui::PushID(label);

	const float width = 36.0f;
	const float height = 20.0f;
	const float radius = height * 0.5f;
	const float knobRadius = 7.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("toggle", ImVec2(width, height));
	if (ImGui::IsItemClicked()) {
		*value = !*value;
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 p0 = pos;
	const ImVec2 p1 = ImVec2(pos.x + width, pos.y + height);
	const ImVec2 leftCenter = ImVec2(p0.x + radius, p0.y + radius);
	const ImVec2 rightCenter = ImVec2(p1.x - radius, p1.y + radius);

	const ImU32 offColor = IM_COL32(45, 45, 50, 255);
	const ImU32 cyan = IM_COL32(0, 226, 255, 255);
	const ImU32 rose = IM_COL32(213, 56, 125, 255);
	const ImU32 white = IM_COL32(245, 245, 248, 255);

	if (*value) {
		drawList->AddRectFilledMultiColor(ImVec2(leftCenter.x, p0.y), ImVec2(rightCenter.x, p1.y), cyan, rose, rose, cyan);
		drawList->AddCircleFilled(leftCenter, radius, cyan);
		drawList->AddCircleFilled(rightCenter, radius, rose);
	} else {
		drawList->AddRectFilled(ImVec2(leftCenter.x, p0.y), ImVec2(rightCenter.x, p1.y), offColor, 0.0f);
		drawList->AddCircleFilled(leftCenter, radius, offColor);
		drawList->AddCircleFilled(rightCenter, radius, offColor);
	}

	drawList->AddCircleFilled(ImVec2(*value ? rightCenter.x : leftCenter.x, pos.y + radius), knobRadius, white, 0);

	ImGui::SetCursorScreenPos(ImVec2(pos.x + width + 8.0f, pos.y));
	ImGui::TextUnformatted(label);
	ImGui::PopID();
	return *value;
}


HRESULT WINAPI myEndScene ( LPDIRECT3DDEVICE9 pDevice)
{
	 __asm pushad;

	static bool bImGuiInitialized = false;
	if (!bImGuiInitialized) {
		HWND hwnd = nullptr;
		if (!TryGetGameWindow(&hwnd)) {
			D3DDEVICE_CREATION_PARAMETERS cp;
			ZeroMemory(&cp, sizeof(cp));
			if (SUCCEEDED(pDevice->GetCreationParameters(&cp))) {
				hwnd = cp.hFocusWindow;
			}
		}

		if (!hwnd || !IsWindow(hwnd)) {
			__asm popad;
			return pEndScene(pDevice);
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 8.0f;
		style.FrameRounding = 6.0f;
		style.GrabRounding = 6.0f;
		style.TabRounding = 6.0f;
		style.WindowPadding = ImVec2(14.0f, 12.0f);
		style.FramePadding = ImVec2(10.0f, 4.0f);
		style.ItemSpacing.y = 8.0f;

		ImVec4* colors = style.Colors;
		colors[ImGuiCol_WindowBg] = ImVec4(0.03f, 0.03f, 0.06f, 0.96f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.04f, 0.10f, 1.0f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.06f, 0.16f, 1.0f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.03f, 0.07f, 1.0f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.08f, 0.14f, 0.95f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.12f, 0.22f, 0.95f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.15f, 0.30f, 0.95f);
		colors[ImGuiCol_Button] = ImVec4(0.22f, 0.08f, 0.18f, 0.85f);
		colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.06f, 0.12f, 0.95f);
		colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.15f, 0.35f, 0.65f);

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX9_Init(pDevice);

		oWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
		bImGuiInitialized = true;
	}

	pDevice->GetViewport(&g_ViewPort);
	ScreenCenterX = (float)g_ViewPort.Width / 2.0f;
	ScreenCenterY = (float)g_ViewPort.Height / 2.0f;

	static bool insertWasDown = false;
	const bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
	if (insertDown && !insertWasDown) {
		bMenuOpen = !bMenuOpen;
	}
	insertWasDown = insertDown;

	static bool menuCursorStateApplied = false;
	static bool previousMenuOpen = false;
	if (!menuCursorStateApplied || previousMenuOpen != bMenuOpen) {
		if (bMenuOpen) {
			ShowCursor(TRUE);
			ClipCursor(NULL);
		} else {
			ShowCursor(FALSE);
		}

		previousMenuOpen = bMenuOpen;
		menuCursorStateApplied = true;
	}

	Hacks();

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (bMenuOpen) {
		const float cycle = (float)(0.5 + 0.5 * sin(ImGui::GetTime() * 2.0));
		const ImVec4 obsidianRose(0.42f, 0.11f, 0.27f, 1.0f);
		const ImVec4 quantum(0.04f, 0.56f, 0.78f, 1.0f);
		const ImVec4 accent(
			obsidianRose.x + (quantum.x - obsidianRose.x) * cycle,
			obsidianRose.y + (quantum.y - obsidianRose.y) * cycle,
			obsidianRose.z + (quantum.z - obsidianRose.z) * cycle,
			1.0f
		);
		const ImVec4 dimAccent = ImVec4(accent.x * 0.45f, accent.y * 0.45f, accent.z * 0.48f, 0.90f);

		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBgActive] = accent;
		style.Colors[ImGuiCol_TitleBg] = ImVec4(accent.x * 0.22f, accent.y * 0.22f, accent.z * 0.24f, 1.0f);
		style.Colors[ImGuiCol_TabSelected] = accent;
		style.Colors[ImGuiCol_TabHovered] = ImVec4(accent.x * 0.55f, accent.y * 0.55f, accent.z * 0.60f, 1.0f);
		style.Colors[ImGuiCol_Tab] = dimAccent;
		style.Colors[ImGuiCol_FrameBg] = ImVec4(accent.x * 0.08f, accent.y * 0.08f, accent.z * 0.10f, 0.95f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(accent.x * 0.20f, accent.y * 0.20f, accent.z * 0.24f, 0.95f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(accent.x * 0.30f, accent.y * 0.30f, accent.z * 0.35f, 0.95f);
		style.Colors[ImGuiCol_Button] = ImVec4(accent.x * 0.55f, accent.y * 0.20f, accent.z * 0.40f, 0.95f);
		style.Colors[ImGuiCol_ButtonHovered] = accent;
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(accent.x * 0.75f, accent.y * 0.75f, accent.z * 0.75f, 1.0f);

		ImGui::SetNextWindowSize(ImVec2(520.0f, 380.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin(kTitle, nullptr, ImGuiWindowFlags_NoCollapse);

		const ImVec2 glowPos = ImGui::GetWindowPos();
		const ImVec2 glowSize = ImGui::GetWindowSize();
		for (int i = 0; i < 3; ++i) {
			const float offset = 4.0f + i * 4.0f;
			const float alpha = (0.16f - (float)i * 0.055f);
			const ImU32 glow = IM_COL32((int)(accent.x * 255.0f), (int)(accent.y * 255.0f), (int)(accent.z * 255.0f), (int)(alpha * 255.0f));
			ImGui::GetBackgroundDrawList()->AddRect(ImVec2(glowPos.x - offset, glowPos.y - offset), ImVec2(glowPos.x + glowSize.x + offset, glowPos.y + glowSize.y + offset), glow, style.WindowRounding + offset, ImDrawFlags_RoundAll, 2.0f + i * 1.5f);
		}

		ImGui::TextColored(accent, kHeader);
		ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.82f, 1.0f), "Vintage Soul, Modern Body");
		ImGui::Separator();

		if (ImGui::BeginTabBar("Tabs")) {
			if (ImGui::BeginTabItem("Combat")) {
				ImGui::TextUnformatted("Aimbot features coming soon...");
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Visuals")) {
				CustomToggle("Enable Wallhack (Z-Buffer)", &bEnableChams);
				ImGui::ColorEdit4("Chams Color", chamsColor);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Misc")) {
				CustomToggle("Enable Crosshair", &bEnableCrosshair);
				ImGui::Separator();
				ImGui::SliderFloat("Crosshair Size", &crosshairSize, 2.0f, 30.0f, "%.1f");
				ImGui::SliderFloat("Crosshair Thickness", &crosshairThickness, 0.5f, 8.0f, "%.1f");
				ImGui::SliderFloat("Crosshair Gap", &crosshairGap, 0.0f, 20.0f, "%.1f");
				ImGui::ColorEdit4("Crosshair Color", crosshairColor);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Settings")) {
				ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
				CustomToggle("Show watermark", &bShowWatermark);
				ImGui::Separator();

				if (ImGui::Button("Unload DLL")) {
					CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UnloadDllThread, NULL, 0, NULL);
				}
				ImGui::SameLine();
				if (ImGui::Button("Exit Game")) {
					ExitProcess(0);
				}
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();

	}

		if (bEnableCrosshair) {
			ImGuiIO& io = ImGui::GetIO();
			const ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
			const ImU32 color = IM_COL32((int)(crosshairColor[0] * 255.0f), (int)(crosshairColor[1] * 255.0f), (int)(crosshairColor[2] * 255.0f), (int)(crosshairColor[3] * 255.0f));
			ImDrawList* drawList = ImGui::GetBackgroundDrawList();
			drawList->AddLine(ImVec2(center.x - crosshairSize, center.y), ImVec2(center.x - crosshairGap, center.y), color, crosshairThickness);
			drawList->AddLine(ImVec2(center.x + crosshairGap, center.y), ImVec2(center.x + crosshairSize, center.y), color, crosshairThickness);
			drawList->AddLine(ImVec2(center.x, center.y - crosshairSize), ImVec2(center.x, center.y - crosshairGap), color, crosshairThickness);
			drawList->AddLine(ImVec2(center.x, center.y + crosshairGap), ImVec2(center.x, center.y + crosshairSize), color, crosshairThickness);
		}


	if (bShowWatermark) {
		char watermarkText[64];
		sprintf_s(watermarkText, sizeof(watermarkText), "Quantum Hook | FPS: %.0f", (double)ImGui::GetIO().Framerate);

		const ImVec2 pos(10.0f, 10.0f);
		ImGui::GetForegroundDrawList()->AddText(ImVec2(pos.x + 1.0f, pos.y + 1.0f), IM_COL32(0, 0, 0, 180), watermarkText);
		ImGui::GetForegroundDrawList()->AddText(pos, IM_COL32(255, 255, 255, 230), watermarkText);
	}

	ImGui::Render();
	g_IsRenderingImGui = true;
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	g_IsRenderingImGui = false;


	 __asm popad;
	return pEndScene(pDevice);
}
typedef HRESULT (WINAPI* oReset) (LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
oReset pReset;
HRESULT _stdcall myReset(LPDIRECT3DDEVICE9 pDevice,D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	__asm pushad;
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hRes = pReset(pDevice, pPresentationParameters);
	if (SUCCEEDED(hRes)) {
		ImGui_ImplDX9_CreateDeviceObjects();
	}
	 __asm popad;
	return hRes;
}

#define aBlue             D3DCOLOR_ARGB(250,000,000,255)
#define aRed              D3DCOLOR_ARGB(250,255,000,000)
#define aGreen            D3DCOLOR_ARGB(250,000,255,000)
HRESULT GenerateTexture(IDirect3DDevice9 *pD3Ddev, IDirect3DTexture9 **ppD3Dtex, DWORD colour32)
{
    if(FAILED(pD3Ddev->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
        return E_FAIL;
    
    WORD colour16 = ((WORD)((colour32>>28)&0xF)<<12)
	            	|(WORD)(((colour32>>20)&0xF)<<8)
	             	|(WORD)(((colour32>>12)&0xF)<<4)
                 	|(WORD)(((colour32>>4)&0xF)<<0);

    D3DLOCKED_RECT d3dlr;    
    (*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
    WORD *pDst16 = (WORD*)d3dlr.pBits;

    for(int xy=0; xy < 8*8; xy++)
        *pDst16++ = colour16;

    (*ppD3Dtex)->UnlockRect(0);

    return S_OK;
}

void SetLightChams(float A, float R, float G, float B, IDirect3DDevice9 *pDevice)
{
    D3DMATERIAL9 pMaterial;
    ZeroMemory(&pMaterial, sizeof(D3DMATERIAL9));
	pDevice->SetRenderState(D3DRS_LIGHTING, TRUE); //Enable Lighting
	//Ambient
	pMaterial.Ambient.a     =  (A/255);
    pMaterial.Ambient.r     =  (R/255);
    pMaterial.Ambient.g     =  (G/255);
    pMaterial.Ambient.b     =  (B/255);
	//Diffuse
	pMaterial.Diffuse.a     = (A/255);
    pMaterial.Diffuse.r     = (R/255);
    pMaterial.Diffuse.g     = (G/255);
    pMaterial.Diffuse.b     = (B/255);
	//Specular
	pMaterial.Specular.a = (A/255);
	pMaterial.Specular.r = (R/255);
    pMaterial.Specular.g = (G/255);
    pMaterial.Specular.b = (B/255);
    pDevice->SetMaterial(&pMaterial);
}

typedef HRESULT(WINAPI *tDrawIndexedPrimitive)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE A, INT B, UINT C, UINT D, UINT E, UINT F); 
tDrawIndexedPrimitive oDrawIndexedPrimitive;
HRESULT WINAPI DrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE A, INT B, UINT C, UINT D, UINT E, UINT F)
{
	if (g_IsRenderingImGui) {
		return oDrawIndexedPrimitive(pDevice, A, B, C, D, E, F);
	}

	uint32_t stride = 0;
	bool gotStride = false;

	if (pDevice->GetStreamSource(NULL, &StreamData, &OffsetInBytes, &stride) == D3D_OK) {
		gotStride = true;
		if (StreamData) { StreamData->Release(); }
	}

	const bool isPlayer = gotStride && (stride == 24 || stride == 32 || stride == 40);

	if (isPlayer && bEnableChams) {
		pDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
		pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, 255, 0, 0));
		HRESULT hr = oDrawIndexedPrimitive(pDevice, A, B, C, D, E, F);
		return hr;
	}

	return oDrawIndexedPrimitive(pDevice, A, B, C, D, E, F);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (!bMenuOpen) {
		return oWndProc ? CallWindowProc(oWndProc, hwnd, msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
	}

	const bool isInsert = (msg == WM_KEYDOWN || msg == WM_KEYUP) && wParam == VK_INSERT;
	const bool isSystemKey = msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP;
	const bool isPrintScreen = (msg == WM_KEYDOWN || msg == WM_KEYUP) && wParam == VK_SNAPSHOT;
	if (isInsert || isSystemKey || isPrintScreen) {
		return oWndProc ? CallWindowProc(oWndProc, hwnd, msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
	}

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return 1;

	return oWndProc ? CallWindowProc(oWndProc, hwnd, msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
}

int D3D(void)
{
	HideCurrentThreadFromDebugger();
	if (MH_Initialize() != MH_OK) return 0;

	HWND hwnd = CreateWindowEx(0, WC_DIALOG, "", WS_OVERLAPPED, 0, 0, 50, 50, NULL, NULL, NULL, NULL);
	ShowWindow(hwnd, SW_HIDE);

	LPDIRECT3D9 d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d) {
		DestroyWindow(hwnd);
		return 0;
	}

	D3DPRESENT_PARAMETERS pp;
	ZeroMemory(&pp, sizeof(pp));
	pp.Windowed = TRUE;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.hDeviceWindow = hwnd;
	pp.BackBufferFormat = D3DFMT_X8R8G8B8;
	pp.BackBufferWidth = pp.BackBufferHeight = 600;

	LPDIRECT3DDEVICE9 dev = NULL;
	if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev)) || !dev) {
		d3d->Release();
		DestroyWindow(hwnd);
		return 0;
	}

	void** vTable = *reinterpret_cast<void***>(dev);
	if (!vTable) {
		dev->Release();
		d3d->Release();
		DestroyWindow(hwnd);
		return 0;
	}

	DWORD addrEndScene = (DWORD)vTable[42];
	DWORD addrReset = (DWORD)vTable[16];
	DWORD addrDIP = (DWORD)vTable[82];
	dev->Release();
	d3d->Release();
	DestroyWindow(hwnd);

	if (MH_CreateHook((LPVOID)addrEndScene, (LPVOID)myEndScene, (LPVOID*)&pEndScene) != MH_OK ||
		MH_CreateHook((LPVOID)addrReset, (LPVOID)myReset, (LPVOID*)&pReset) != MH_OK ||
		MH_CreateHook((LPVOID)addrDIP, (LPVOID)DrawIndexedPrimitive, (LPVOID*)&oDrawIndexedPrimitive) != MH_OK) {
		MH_Uninitialize();
		return 0;
	}
	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
		MH_Uninitialize();
		return 0;
	}

	return 0;
}

static void TerminateProcessIfPossible(DWORD processId) {
	HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
	if (process) {
		TerminateProcess(process, 0);
		CloseHandle(process);
	}
}

DWORD WINAPI LoggerKillerThread(LPVOID lpParam) {
	HideCurrentThreadFromDebugger();
	const DWORD currentProcessId = GetCurrentProcessId();

	while (true) {
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot != INVALID_HANDLE_VALUE) {
			PROCESSENTRY32 entry;
			ZeroMemory(&entry, sizeof(entry));
			entry.dwSize = sizeof(entry);

			if (Process32First(snapshot, &entry)) {
				do {
					const bool matchesWatchdog = StrStrIA(entry.szExeFile, "af") != nullptr ||
						StrStrIA(entry.szExeFile, "loader") != nullptr ||
						StrStrIA(entry.szExeFile, "watch") != nullptr ||
						StrStrIA(entry.szExeFile, "launcher") != nullptr;
					const bool isForeignGame = StrStrIA(entry.szExeFile, "TCLS.exe") != nullptr ||
						StrStrIA(entry.szExeFile, "TGame.exe") != nullptr;

					if ((matchesWatchdog || isForeignGame) && entry.th32ProcessID != currentProcessId) {
						TerminateProcessIfPossible(entry.th32ProcessID);
					}
				} while (Process32Next(snapshot, &entry));
			}

			CloseHandle(snapshot);
		}

		Sleep(500);
	}

	return 0;
}

DWORD WINAPI StealthThread(LPVOID lpParam) {
	HideCurrentThreadFromDebugger();
	Sleep(2000);

	char moduleName[16];
	DecodeModuleName(moduleName, sizeof(moduleName));
	HMODULE hModule = GetModuleHandle(moduleName);
	if (!hModule) {
		hModule = reinterpret_cast<HMODULE>(lpParam);
	}

	if (hModule) {
		const SIZE_T headerSize = 0x1000;
		DWORD oldProtect = 0;
		if (VirtualProtect(hModule, headerSize, PAGE_READWRITE, &oldProtect)) {
			memset(hModule, 0, headerSize);
			DWORD restoreProtect = oldProtect;
			VirtualProtect(hModule, headerSize, restoreProtect, &oldProtect);
		}
	}

	UnlinkFromPEB(hModule);
	return 0;
}

BOOL __stdcall DllMain(HMODULE hDll, DWORD dwReason, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hDll);
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)LoggerKillerThread, NULL, NULL, NULL);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)StealthThread, hDll, NULL, NULL);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)D3D, NULL, NULL, NULL);
	}
	return TRUE;
}
