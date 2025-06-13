#pragma once

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>

#pragma comment(lib, "d3d9.lib")

#include <string>
#include <iostream>

enum tabChanger
{
	MAIN_MENU,
	NEW_FILE_MENU,
	OPEN_FILE_MENU,
	UPDATE_MENU,
	ADDONS_MENU,
	SETTINGS_MENU,
	EXIT_MENU
};

namespace gui
{
	constexpr int WIDTH = 700; 
	constexpr int HEIGHT = 400; 

	inline int tab = 0; 
	inline bool isRunning = true;

	inline HWND window = nullptr;
	inline WNDCLASSEX windowClass = { };

	inline POINTS position = { };
	inline PDIRECT3D9 d3d = nullptr;

	inline LPDIRECT3DDEVICE9 device = nullptr;
	inline D3DPRESENT_PARAMETERS presentParameters = { };

	void MainMenu() noexcept;
	void NewFileMenu() noexcept;
	void OpenFileMenu() noexcept;
	void SettingsMenu() noexcept;
	void AddonsMenu() noexcept;
	void UpdateMenu() noexcept;
	void ExitMenu() noexcept;

	void CreateHWindow(const char* windowName) noexcept;
	void DestroyHWindow() noexcept;

	bool CreateDevice() noexcept;
	void ResetDevice() noexcept;
	void DestroyDevice() noexcept;

	void CreateImGui() noexcept;
	void DestroyImGui() noexcept;

	void BeginRender() noexcept;
	void EndRender() noexcept;
	void Render() noexcept;
}

