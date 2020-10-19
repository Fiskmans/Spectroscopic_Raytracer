#include "stdafx.h"
#include "WindowData.h"

#include "GraphicEngine.h"
#include "RenderManager.h"
#include "DirectX11Framework.h"
#include "WindowHandler.h"
#include "Camera.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "RayScheduler.h"
#include "RayRenderer.h"

void SaveImGuiStyle()
{
	static_assert(sizeof(char) == 1, "Double check");
	std::ofstream stream;
	stream.open("ImGuiStyle.ini", std::ios::binary | std::ios::out);
	stream.write(reinterpret_cast<char*>(&ImGui::GetStyle()), sizeof(*(&ImGui::GetStyle())));
}

void LoadOrDefaultImGuiStyle()
{
	static_assert(sizeof(char) == 1, "Double check");
	std::ifstream stream;
	stream.open("ImGuiStyle.ini", std::ios::binary | std::ios::in);
	if (stream)
	{
		stream.read(reinterpret_cast<char*>(&ImGui::GetStyle()), sizeof(*(&ImGui::GetStyle())));
	}
	else
	{
		ImGui::StyleColorsDark();
	}
}

int Run()
{
	const wchar_t* commLine = GetCommandLineW();
	int argc;
	LPWSTR* argv = CommandLineToArgvW(commLine, &argc);
	for (size_t i = 0; i < argc; i++)
	{
		DebugTools::CommandLineFlags.emplace(argv[i]);
	}

	long long startTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

	AllocConsole();

	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stderr);

	Logger::SetFilter(Logger::Type::AnyGame | Logger::Type::AnyWarning | Logger::Type::AllSystem & ~Logger::Type::AnyVerbose);
	Logger::SetHalting(Logger::Type::SystemCrash);


	Logger::Map(Logger::Type::AnyGame, "info");
	Logger::Map(Logger::Type::AnySystem, "system");
	Logger::SetColor(Logger::Type::AnyError, FOREGROUND_RED | FOREGROUND_INTENSITY);
	Logger::SetColor(Logger::Type::AnyWarning, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	Logger::SetColor(Logger::Type::AnyInfo, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	Logger::SetColor(Logger::Type::SystemNetwork, FOREGROUND_GREEN | FOREGROUND_BLUE);


	Window::WindowData windowData;
	windowData.myX = 0;
	windowData.myY = 0;
	windowData.myWidth = 800;
	windowData.myHeight = 600;

	CGraphicsEngine engine;
	RayScheduler* scheduler = nullptr;
	Camera* mainCamera = new Camera();

	mainCamera->Init(120, { 1600,1200 }, 0.1f, 10000);
	mainCamera->SetPosition(V3F(0,0,-100));
	mainCamera->LookAt(V3F(0, 0, 0));


	if (!engine.Init(windowData))
	{
		SYSCRASH("Could not start engine");
		return -1;
	}

	RayRenderer renderer;
	renderer.AddModel("data/models/test.fbx");
	renderer.SetBoundingSize(150.0);

	{
		FullscreenTexture targetTexture = engine.GetRendreManarger()->GetOutputTexture();
		ID3D11ShaderResourceView* view = targetTexture.GetResourceView();
		ID3D11Resource* res;
		view->GetResource(&res);
		ID3D11Texture2D* tex;

		HRESULT result = res->QueryInterface(&tex);
		if (SUCCEEDED(result))
		{
			D3D11_TEXTURE2D_DESC desc;
			tex->GetDesc(&desc);

			scheduler = new RayScheduler(
				desc.Width, 
				desc.Height, 
				tex, 
				mainCamera, 
				engine.GetFrameWork()->GetContext(),
				renderer);

			tex->Release();
		}

		res->Release();
	}
	if (!scheduler)
	{
		SYSCRASH("Could not start scheduler");
		return EXIT_FAILURE;
	}


	engine.GetRendreManarger()->myCamera = mainCamera;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	LoadOrDefaultImGuiStyle();
	ImGui_ImplDX11_Init(engine.GetFrameWork()->GetDevice(), engine.GetFrameWork()->GetContext());
	ImGui_ImplWin32_Init(engine.GetWindowHandler()->GetWindowHandle());

	MSG windowMessage;
	WIPE(windowMessage);
	float clearColor[4] = { 0.8f,0.36f,0.7f,1.f };
	bool shutdown = false;
	while (!shutdown)
	{
		{
			PERFORMANCETAG("Winmessage Parsing");
			while (PeekMessage(&windowMessage, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&windowMessage);
				DispatchMessage(&windowMessage);

				if (windowMessage.message == WM_QUIT || windowMessage.message == WM_DESTROY)
				{
					shutdown = true;
				}
			}
		}
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		WindowControl::DrawWindowControl();
		WindowControl::Window("Console log", []()
			{
				Logger::ImGuiLog();
			});
		DirectX11Framework::Imgui();
		WindowControl::Window("Diagnostic", []()
			{
				Tools::TimeTree* at = Tools::GetTimeTreeRoot();
				static bool accumulative = true;
				ImGui::Checkbox("Accumulative", &accumulative);
				Tools::DrawTimeTree(at);
				if (!accumulative)
				{
					Tools::FlushTimeTree();
				}
			});

		{
			PERFORMANCETAG("Engine run");
			engine.BeginFrame(clearColor);
			scheduler->Imgui();
			scheduler->Update();
			scheduler->PushToHardware();
			engine.RenderFrame();
		}


		{
			PERFORMANCETAG("Main loop");
			{
				PERFORMANCETAG("Imgui Drawing [old]");
				ImGui::Render();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			}
			{
				PERFORMANCETAG("End frame [old]");
				engine.EndFrame();
			}
		}
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();


	return EXIT_SUCCESS;
}