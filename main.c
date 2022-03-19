#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>
#include <stdlib.h>

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);

void DoPerlinNoise1D(int nCount, float* fSeed, int nOctaves, float fBias, float* fOutput)
{
    for (int x = 0; x < nCount; x++)
    {
        float fNoise = 0.0f;
        float fScale = 1.0f;
        float fScaleAccumulator = 0.0f;

        for (int o = 0; o < nOctaves; o++)
        {
            int nPitch = nCount >> o;
            int nSample1 = (x / nPitch) * nPitch;
            int nSample2 = (nSample1 + nPitch) % nCount;

            float fBlend = (float)(x - nSample1) / (float)nPitch;
            float fSample = (1.0f - fBlend) * fSeed[nSample1] + fBlend * fSeed[nSample2];

            fNoise += fSample * fScale;
            fScaleAccumulator += fScale;
            fScale /= fBias;
        }

        fOutput[x] = fNoise / fScaleAccumulator;
    }
}

void DoPerlinNoise2D(int nWidth, int nHeight, float* fSeed, int nOctaves, float fBias, float* fOutput)
{
    for (int x = 0; x < nWidth; x++)
        for (int y = 0; y < nHeight; y++)
        {
            float fNoise = 0.0f;
            float fScale = 1.0f;
            float fScaleAccumulator = 0.0f;

            for (int o = 0; o < nOctaves; o++)
            {
                int nPitch = nWidth >> o;

                int nSampleX1 = (x / nPitch) * nPitch;
                int nSampleY1 = (y / nPitch) * nPitch;

                int nSampleX2 = (nSampleX1 + nPitch) % nWidth;
                int nSampleY2 = (nSampleY1 + nPitch) % nWidth;

                float fBlendX = (float)(x - nSampleX1) / (float)nPitch;
                float fBlendY = (float)(y - nSampleY1) / (float)nPitch;

                float fSampleT = (1.0f - fBlendX) * fSeed[nSampleY1 * nWidth + nSampleX1] + fBlendX * fSeed[nSampleY1 * nWidth + nSampleX2];
                float fSampleB = (1.0f - fBlendX) * fSeed[nSampleY2 * nWidth + nSampleX1] + fBlendX * fSeed[nSampleY2 * nWidth + nSampleX2];

                fNoise += (fBlendY * (fSampleB - fSampleT) + fSampleT) * fScale;
                fScaleAccumulator += fScale;
                fScale /= fBias;
            }

            fOutput[y * nWidth + x] = fNoise / fScaleAccumulator;
        }
}

const int nScreenWidth = 1024;
const int nScreenHeight = 768;

#define OUTPUT_SIZE 1024
#define MAX_OCTAVE_COUNT 9

float fNoiseSeed1D[OUTPUT_SIZE];
float fPerlinNoise1D[OUTPUT_SIZE];

#define OUTPUT_WIDTH 1024
#define OUTPUT_HEIGHT 768

float fNoiseSeed2D[OUTPUT_WIDTH * OUTPUT_HEIGHT];
float fPerlinNoise2D[OUTPUT_WIDTH * OUTPUT_HEIGHT];

int nOctaveCount = 1;
float fScalingBias = 2.0f;
int nMode = 1;

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    srand(time(NULL));

    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;

    /* register window class */
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GLSample";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;


    if (!RegisterClassEx(&wcex))
        return 0;

    /* create main window */
    hwnd = CreateWindowEx(0,
                          "GLSample",
                          "Perlin Noise",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          nScreenWidth,
                          nScreenHeight,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hwnd, nCmdShow);

    /* enable OpenGL for the window */
    EnableOpenGL(hwnd, &hDC, &hRC);

    glOrtho(0, nScreenWidth, 0, nScreenHeight, 0, 1);

    for (int i = 0; i < OUTPUT_SIZE; i++)
        fNoiseSeed1D[i] = (float)rand() / (float)RAND_MAX;

    for (int i = 0; i < OUTPUT_WIDTH * OUTPUT_HEIGHT; i++)
        fNoiseSeed2D[i] = (float)rand() / (float)RAND_MAX;

    /* program main loop */
    while (!bQuit)
    {
        /* check for messages */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            /* handle or dispatch messages */
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            /* OpenGL animation code goes here */

            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glPushMatrix();

            if (nOctaveCount == MAX_OCTAVE_COUNT)
                nOctaveCount = 1;

            if (fScalingBias < 0.2f)
                fScalingBias = 0.2f;

            if (nMode == 1)
            {
                DoPerlinNoise1D(OUTPUT_SIZE, fNoiseSeed1D, nOctaveCount, fScalingBias, fPerlinNoise1D);

                glBegin(GL_LINES);
                for (int x = 0; x < OUTPUT_SIZE; x++)
                {
                    int y = fPerlinNoise1D[x] * (float)nScreenHeight / 2.0f + (float)nScreenHeight / 2.0f;

                    glColor3f(0.0f, 1.0f, 0.0f);
                    glVertex2f(x, y);

                    glColor3f(0.0f, 1.0f, 0.0f);
                    glVertex2f(x, nScreenHeight / 2);
                }
                glEnd();
            }
            else if (nMode == 2)
            {
                DoPerlinNoise2D(OUTPUT_WIDTH, OUTPUT_HEIGHT, fNoiseSeed2D, nOctaveCount, fScalingBias, fPerlinNoise2D);

                glBegin(GL_POINTS);
                for (int x = 0; x < OUTPUT_WIDTH; x++)
                    for (int y = 0; y < OUTPUT_HEIGHT; y++)
                    {
                        float col = fPerlinNoise2D[y * OUTPUT_WIDTH + x];

                        glColor3f(col + 0.15f, col + 0.15f, col + 0.15f);
                        glVertex2f(x, y);
                    }
                glEnd();
            }

            glPopMatrix();

            SwapBuffers(hDC);

            Sleep (1);
        }
    }

    /* shutdown OpenGL */
    DisableOpenGL(hwnd, hDC, hRC);

    /* destroy the window explicitly */
    DestroyWindow(hwnd);

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
        break;

        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;

                case VK_SPACE:
                    nOctaveCount++;
                    break;

                case L'Z':
                    if (nMode == 1)
                    {
                        for (int i = 0; i < OUTPUT_SIZE; i++)
                            fNoiseSeed1D[i] = (float)rand() / (float)RAND_MAX;
                    }
                    else if (nMode == 2)
                    {
                        for (int i = 0; i < OUTPUT_WIDTH * OUTPUT_HEIGHT; i++)
                            fNoiseSeed2D[i] = (float)rand() / (float)RAND_MAX;
                    }

                    break;

                case L'Q':
                    fScalingBias += 0.2f;
                    break;

                case L'A':
                    fScalingBias -= 0.2f;
                    break;

                case L'1':
                    nMode = 1;
                    nOctaveCount = 1;

                    printf("1");

                    for (int i = 0; i < OUTPUT_SIZE; i++)
                        fNoiseSeed1D[i] = (float)rand() / (float)RAND_MAX;

                    break;

                case L'2':
                    nMode = 2;
                    nOctaveCount = 1;

                    printf("2");

                    for (int i = 0; i < OUTPUT_WIDTH * OUTPUT_HEIGHT; i++)
                        fNoiseSeed2D[i] = (float)rand() / (float)RAND_MAX;

                    break;
            }
        }
        break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC(hwnd);

    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}
