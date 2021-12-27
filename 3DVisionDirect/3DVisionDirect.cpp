// include the basic windows header files and the Direct3D header file
#include <windows.h>
#include <d3d9.h>
#include <D3dx9tex.h>

#include "nvapi.h"
#include "nvapi_lite_stereo.h"

// include the Direct3D Library file
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE         g_hInst = nullptr;
HWND              g_hWnd = nullptr;

LPDIRECT3D9       g_d3d = nullptr;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 g_dev = nullptr;    // the pointer to the device class

StereoHandle	g_StereoHandle;
LONG			g_ScreenWidth = 2560;
LONG			g_ScreenHeight = 1440;

// function prototypes
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitStereo();
HRESULT InitDevice();
HRESULT ActivateStereo();
void CleanupDevice();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void RenderFrame();

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;

    if (FAILED(InitStereo()))
        return 0;

    // set up and initialize Direct3D
    if (FAILED(InitDevice())) {
        CleanupDevice();
        return 0;
    }

    if (FAILED(ActivateStereo()))
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            RenderFrame();
        }
    }

    CleanupDevice();

    return (int)msg.wParam;
}

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"Directx9WindowClass";
    wcex.hIconSm = nullptr;
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, g_ScreenWidth, g_ScreenHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindow(L"Directx9WindowClass", L"Direct3D 9",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
        nullptr);
    if (!g_hWnd)
        return E_FAIL;

    ShowWindow(g_hWnd, nCmdShow);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Setup nvapi, and enable stereo by direct mode for the app.
// This must be called before the Device is created for Direct Mode to work.
//--------------------------------------------------------------------------------------
HRESULT InitStereo()
{
    NvAPI_Status status;

    status = NvAPI_Initialize();
    if (FAILED(status))
        return status;

    // The entire point is to show stereo.  
    // If it's not enabled in the control panel, let the user know.
    NvU8 stereoEnabled;
    status = NvAPI_Stereo_IsEnabled(&stereoEnabled);
    if (FAILED(status) || !stereoEnabled)
    {
        MessageBox(g_hWnd, L"3D Vision is not enabled. Enable it in the NVidia Control Panel.", L"Error", MB_OK);
        return status;
    }

    status = NvAPI_Stereo_SetDriverMode(NVAPI_STEREO_DRIVER_MODE_DIRECT);
    if (FAILED(status))
        return status;

    return status;
}

//--------------------------------------------------------------------------------------
// Activate stereo for the given device.
// This must be called after the device is created.
//--------------------------------------------------------------------------------------
HRESULT ActivateStereo()
{
    NvAPI_Status status;

    status = NvAPI_Stereo_CreateHandleFromIUnknown(g_dev, &g_StereoHandle);
    if (FAILED(status))
        return status;

    return status;
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

        // Note that this tutorial does not handle resizing (WM_SIZE) requests,
        // so we created the window without the resize border.

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}




// this function initializes and prepares Direct3D for use
HRESULT InitDevice()
{
    g_d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface

    D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

    ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
    d3dpp.Windowed = FALSE;    // program fullscreen, not windowed
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
    d3dpp.BackBufferCount = 1;
    d3dpp.hDeviceWindow = g_hWnd;    // set the window to be used by Direct3D
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;    // set the back buffer format to 32-bit
    d3dpp.BackBufferWidth = g_ScreenWidth;    // set the width of the buffer
    d3dpp.BackBufferHeight = g_ScreenHeight;    // set the height of the buffer


    // create a device class using this information and the info from the d3dpp stuct
    HRESULT hr = g_d3d->CreateDevice(D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        g_hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp,
        &g_dev);

    return hr;
}

// this is the function used to render a single frame
void RenderFrame()
{
    NvAPI_Status status;
    IDirect3DSurface9* gImageSrcLeftRight; // SBS image
    g_dev->CreateOffscreenPlainSurface(g_ScreenWidth, g_ScreenHeight, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &gImageSrcLeftRight, NULL);
    D3DXLoadSurfaceFromFile(gImageSrcLeftRight, NULL, NULL, L"snapshot.png", NULL, D3DX_FILTER_NONE, 0, NULL);
    
    

    status = NvAPI_Stereo_SetActiveEye(g_StereoHandle, NVAPI_STEREO_EYE_LEFT);
    if (SUCCEEDED(status)) {
        g_dev->BeginScene();    // begins the 3D scene
        IDirect3DSurface9* pBackBuffer;
        g_dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

        RECT Rect = { 0, 0, g_ScreenWidth, g_ScreenHeight };
        g_dev->StretchRect(gImageSrcLeftRight, &Rect, pBackBuffer, &Rect, D3DTEXF_NONE);
        pBackBuffer->Release();
        g_dev->EndScene();    // ends the 3D scene
    }

    status = NvAPI_Stereo_SetActiveEye(g_StereoHandle, NVAPI_STEREO_EYE_RIGHT);
    if (SUCCEEDED(status)) {
        g_dev->BeginScene();    // begins the 3D scene
        IDirect3DSurface9* pBackBuffer;
        g_dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
        RECT Rect = { 0, 0, g_ScreenWidth, g_ScreenHeight };
        g_dev->StretchRect(gImageSrcLeftRight, &Rect, pBackBuffer, &Rect, D3DTEXF_NONE);
        pBackBuffer->Release();
        g_dev->EndScene();    // ends the 3D scene
    }

    gImageSrcLeftRight->Release();

    g_dev->Present(NULL, NULL, NULL, NULL);   // displays the created frame on the screen
}

// this is the function that cleans up Direct3D and COM
void CleanupDevice()
{
    g_dev->Release();    // close and release the 3D device
    g_d3d->Release();    // close and release Direct3D
}