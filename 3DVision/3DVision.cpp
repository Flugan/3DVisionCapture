// include the basic windows header files and the Direct3D header file
#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>

// include the Direct3D Library file
#pragma comment (lib, "d3d9.lib")

// global declarations
LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3ddev;    // the pointer to the device class

// function prototypes
void initD3D(HWND hWnd);    // sets up and initializes Direct3D
void render_frame(void);    // renders a single frame
void cleanD3D(void);    // closes Direct3D and releases memory
void render3D(void);

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// define the screen resolution
#define SCREEN_WIDTH  2560
#define SCREEN_HEIGHT 1440

IDirect3DSurface9* gImageSrcLeft; // Left Source image surface in video memory
IDirect3DSurface9* gImageSrcRight; // Right Source image Surface in video memory

int gImageWidth = SCREEN_WIDTH; // Source image width
int gImageHeight = SCREEN_HEIGHT; // Source image height

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    HWND hWnd;
    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    // wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"WindowClass";

    RegisterClassEx(&wc);

    hWnd = CreateWindowEx(NULL,
        L"WindowClass",
        L"3D Vision",
        WS_POPUP,    // fullscreen values // WS_EX_TOPMOST | 
        0, 0,    // the starting x and y positions should be 0
        SCREEN_WIDTH, SCREEN_HEIGHT,    // set window to new resolution
        NULL,
        NULL,
        hInstance,
        NULL);

    ShowWindow(hWnd, nCmdShow);

    // set up and initialize Direct3D
    initD3D(hWnd);

    // enter the main loop:

    MSG msg;

    while (TRUE)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            break;
        }

        render_frame();
    }

    // clean up DirectX and COM
    cleanD3D();

    return msg.wParam;
}


// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    } break;
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return 0;
    } break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}


// this function initializes and prepares Direct3D for use
void initD3D(HWND hWnd)
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface

    D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

    ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
    d3dpp.Windowed = FALSE;    // program fullscreen, not windowed
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
    d3dpp.BackBufferCount = 1;
    d3dpp.hDeviceWindow = hWnd;    // set the window to be used by Direct3D
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;    // set the back buffer format to 32-bit
    d3dpp.BackBufferWidth = SCREEN_WIDTH;    // set the width of the buffer
    d3dpp.BackBufferHeight = SCREEN_HEIGHT;    // set the height of the buffer


    // create a device class using this information and the info from the d3dpp stuct
    d3d->CreateDevice(D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp,
        &d3ddev);

    d3ddev->CreateOffscreenPlainSurface(gImageWidth, gImageHeight, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &gImageSrcLeft, NULL);
    d3ddev->CreateOffscreenPlainSurface(gImageWidth, gImageHeight, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &gImageSrcRight, NULL);

    d3ddev->ColorFill(gImageSrcLeft, NULL, D3DCOLOR_XRGB(0, 0, 0));
    d3ddev->ColorFill(gImageSrcRight, NULL, D3DCOLOR_XRGB(255, 255, 255));
}

// this is the function used to render a single frame
void render_frame(void)
{
    d3ddev->BeginScene();    // begins the 3D scene

    IDirect3DSurface9* gImageSrc = NULL; // Source stereo image beeing created

    d3ddev->CreateOffscreenPlainSurface(
        gImageWidth * 2, // Stereo width is twice the source width
        gImageHeight + 1, // Stereo height add one raw to encode signature
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, // Surface is in video memory
        &gImageSrc, NULL);

    // Blit left src image to left side of stereo
    RECT srcRect = { 0, 0, gImageWidth, gImageHeight };
    RECT destRect = { 0, 0, gImageWidth, gImageHeight };
    d3ddev->StretchRect(gImageSrcLeft, &srcRect, gImageSrc, &destRect, D3DTEXF_LINEAR);

    // Blit right src image to right side of stereo
    destRect = { gImageWidth, 0, 2 * gImageWidth, gImageHeight };
    d3ddev->StretchRect(gImageSrcRight, &srcRect, gImageSrc, &destRect, D3DTEXF_LINEAR);

    // Stereo Blit defines
#define NVSTEREO_IMAGE_SIGNATURE 0x4433564e //NV3D

    typedef struct _Nv_Stereo_Image_Header
    {
        unsigned int dwSignature;
        unsigned int dwWidth;
        unsigned int dwHeight;
        unsigned int dwBPP;
        unsigned int dwFlags;
    } NVSTEREOIMAGEHEADER, * LPNVSTEREOIMAGEHEADER;

    // ORed flags in the dwFlags fiels of the _Nv_Stereo_Image_Header structure above
#define SIH_SWAP_EYES 0x00000001
#define SIH_SCALE_TO_FIT 0x00000002

    // Lock the stereo image
    D3DLOCKED_RECT lr;
    gImageSrc->LockRect(&lr, NULL, 0);

    // write stereo signature in the last raw of the stereo image
    LPNVSTEREOIMAGEHEADER pSIH =
        (LPNVSTEREOIMAGEHEADER)(((unsigned char*)lr.pBits) + (lr.Pitch * gImageHeight));

    // Update the signature header values
    pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
    pSIH->dwBPP = 32;
    pSIH->dwFlags = SIH_SWAP_EYES; // Src image has left on left and right on right
    pSIH->dwWidth = gImageWidth * 2;
    pSIH->dwHeight = gImageHeight;

    // Unlock surface
    gImageSrc->UnlockRect();

    IDirect3DSurface9* pBackBuffer;
    d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

    RECT codedRect = { 0, 0, gImageWidth * 2, gImageHeight + 1 };
    RECT backRect = { 0, 0, gImageWidth, gImageHeight };
    d3ddev->StretchRect(gImageSrc, &codedRect, pBackBuffer, &backRect, D3DTEXF_LINEAR);

    pBackBuffer->Release();
    gImageSrc->Release();

    d3ddev->EndScene();    // ends the 3D scene

    d3ddev->Present(NULL, NULL, NULL, NULL);   // displays the created frame on the screen
}

// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
    gImageSrcLeft->Release();
    gImageSrcRight->Release();

    d3ddev->Release();    // close and release the 3D device
    d3d->Release();    // close and release Direct3D
}