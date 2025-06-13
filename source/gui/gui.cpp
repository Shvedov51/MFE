#include "gui.h"
#include "../vars.h"

#include "imfilebrowser.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU)
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter);
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;
			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 20)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);

		}

	}return 0;

	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}


void gui::CreateHWindow(const char* windowName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = "classwayzer";
	windowClass.hIconSm = 0;

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	int posX = (screenWidth - WIDTH) / 2;
	int posY = (screenHeight - HEIGHT) / 2;

	RegisterClassEx(&windowClass);

	wchar_t wWindowName[256];
	mbstowcs_s(nullptr, wWindowName, windowName, 256);

	window = CreateWindowExW(
		WS_EX_LAYERED,
		L"classwayzer",
		wWindowName,
		WS_POPUP,
		posX,
		posY,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	SetLayeredWindowAttributes(window, RGB(0, 0, 0), 0, LWA_COLORKEY);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	io.Fonts->AddFontDefault();
	io.Fonts->GetGlyphRangesCyrillic();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

ImGui::FileBrowser fileDialog;

void gui::Render() noexcept 
{
	ImGui::StyleColorsDark();
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin("MFE", &isRunning, ImGuiWindowFlags_NoResize |ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);

	switch (tab)
	{
	case MAIN_MENU:
		MainMenu();
		break;
	case NEW_FILE_MENU:
		NewFileMenu();
		break;
	case OPEN_FILE_MENU:
		OpenFileMenu();
		break;
	case SETTINGS_MENU:
		SettingsMenu();
		break;
	case ADDONS_MENU:
		AddonsMenu();
		break;
	case UPDATE_MENU:
		UpdateMenu();
		break;
	case EXIT_MENU:
		ExitMenu();
		break;
	}

	ImGui::End();

	fileDialog.Display();

	if (fileDialog.HasSelected())
	{
		std::cout << "Selected filename" << fileDialog.GetSelected().string() << std::endl;
		fileDialog.ClearSelected();
	}
}

void gui::MainMenu() noexcept
{
	if (ImGui::Button("New file", ImVec2(120, 40)))
	{
		tab = NEW_FILE_MENU;
	}
	if (ImGui::Button("Open file", ImVec2(120, 40)))
	{
		tab = OPEN_FILE_MENU;
	}
	if (ImGui::Button("Settings", ImVec2(120, 40)))
	{
		tab = SETTINGS_MENU;
	}
	if (ImGui::Button("Addons", ImVec2(120, 40)))
	{
		tab = ADDONS_MENU;
	}
	if (ImGui::Button("Update", ImVec2(120, 40)))
	{
		tab = UPDATE_MENU;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		tab = EXIT_MENU;
	}
}

void gui::NewFileMenu() noexcept
{
	static char nameFile[64] = { 0 };
	ImGui::InputText("Write the name of new file", nameFile, 64);

	if (ImGui::Button("Create file", ImVec2(120, 40)))
	{
		
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		tab = MAIN_MENU;
	}
}

void gui::OpenFileMenu() noexcept
{

	fileDialog.SetTitle("title");
	fileDialog.SetTypeFilters({ ".h", ".cpp" });

	fileDialog.Open();

	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		tab = MAIN_MENU;
	}
}

void gui::SettingsMenu() noexcept
{
	const char* lang[] = {"English", "Russian"};
	ImGui::Combo("Change language", &globals::language, lang, ARRAYSIZE(lang));

	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		tab = MAIN_MENU;
	}
}

void gui::AddonsMenu() noexcept
{
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		tab = MAIN_MENU;
	}
}

void gui::UpdateMenu() noexcept
{
	ImGui::Text(globals::version);
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		tab = MAIN_MENU;
	}
}

void gui::ExitMenu() noexcept
{
	ImGui::SetCursorPosX(100);
	ImGui::Text("Are u sure");
	if (ImGui::Button("Yes, exit",ImVec2(120, 40)))
	{
		gui::isRunning = false;
	}
	ImGui::SetCursorPos(ImVec2(140, 44));
	if (ImGui::Button("No, return", ImVec2(120, 40)))
	{
		tab = MAIN_MENU;
	}
}

