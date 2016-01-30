/************************************************************************************
 Filename    :   Win32_GLAppUtil.h
 Content     :   OpenGL and Application/Window setup functionality for RoomTiny
 Created     :   October 20th, 2014
 Author      :   Tom Heath
 Copyright   :   Copyright 2014 Oculus, LLC. All Rights reserved.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 *************************************************************************************/

#include <GL/CAPI_GLE.h>
#include <Extras/OVR_Math.h>
#include <Kernel/OVR_Log.h>
#include "OVR_CAPI_GL.h"

 using namespace OVR;

#ifndef VALIDATE
    #define VALIDATE(x, msg) if (!(x)) { MessageBoxA(NULL, (msg), "OculusRoomTiny", MB_ICONERROR | MB_OK); exit(-1); }
#endif

//---------------------------------------------------------------------------------------
struct DepthBuffer
{
    GLuint        texId;

    DepthBuffer(Sizei size, int sampleCount)
    {
        OVR_ASSERT(sampleCount <= 1); // The code doesn't currently handle MSAA textures.

        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLenum internalFormat = GL_DEPTH_COMPONENT24;
        GLenum type = GL_UNSIGNED_INT;
        if (GLE_ARB_depth_buffer_float)
        {
            internalFormat = GL_DEPTH_COMPONENT32F;
            type = GL_FLOAT;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.w, size.h, 0, GL_DEPTH_COMPONENT, type, NULL);
    }
    ~DepthBuffer()
    {
        if (texId)
        {
            glDeleteTextures(1, &texId);
            texId = 0;
        }
    }
};

//--------------------------------------------------------------------------
struct TextureBuffer
{
    ovrHmd              hmd;
    ovrSwapTextureSet*  TextureSet;
    GLuint              texId;
    GLuint              fboId;
    Sizei               texSize;

    TextureBuffer(ovrHmd hmd, bool rendertarget, bool displayableOnHmd, Sizei size, int mipLevels, unsigned char * data, int sampleCount) :
        hmd(hmd),
        TextureSet(nullptr),
        texId(0),
        fboId(0),
        texSize(0, 0)
    {
        OVR_ASSERT(sampleCount <= 1); // The code doesn't currently handle MSAA textures.

        texSize = size;

        if (displayableOnHmd)
        {
            // This texture isn't necessarily going to be a rendertarget, but it usually is.
            OVR_ASSERT(hmd); // No HMD? A little odd.
            OVR_ASSERT(sampleCount == 1); // ovr_CreateSwapTextureSetD3D11 doesn't support MSAA.

            ovrResult result = ovr_CreateSwapTextureSetGL(hmd, GL_SRGB8_ALPHA8, size.w, size.h, &TextureSet);

            if(OVR_SUCCESS(result))
            {
                for (int i = 0; i < TextureSet->TextureCount; ++i)
                {
                    ovrGLTexture* tex = (ovrGLTexture*)&TextureSet->Textures[i];
                    glBindTexture(GL_TEXTURE_2D, tex->OGL.TexId);

                    if (rendertarget)
                    {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    }
                    else
                    {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    }
                }
            }
        }
        else
        {
            glGenTextures(1, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);

            if (rendertarget)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }

            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, texSize.w, texSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }

        if (mipLevels > 1)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glGenFramebuffers(1, &fboId);
    }

    ~TextureBuffer()
    {
        if (TextureSet)
        {
            ovr_DestroySwapTextureSet(hmd, TextureSet);
            TextureSet = nullptr;
        }
        if (texId)
        {
            glDeleteTextures(1, &texId);
            texId = 0;
        }
        if (fboId)
        {
            glDeleteFramebuffers(1, &fboId);
            fboId = 0;
        }
    }

    Sizei GetSize() const
    {
        return texSize;
    }

    void SetAndClearRenderSurface(DepthBuffer* dbuffer)
    {
        auto tex = reinterpret_cast<ovrGLTexture*>(&TextureSet->Textures[TextureSet->CurrentIndex]);

        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->OGL.TexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dbuffer->texId, 0);

        glViewport(0, 0, texSize.w, texSize.h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_FRAMEBUFFER_SRGB);
    }

    void UnsetRenderSurface()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }
};

//-------------------------------------------------------------------------------------------
struct OGL
{
    static const bool       UseDebugContext = false;

    HWND                    Window;
    HDC                     hDC;
    HGLRC                   WglContext;
    OVR::GLEContext         GLEContext;
    bool                    Running;
    bool                    Key[256];
    int                     WinSizeW;
    int                     WinSizeH;
    GLuint                  fboId;
    HINSTANCE               hInstance;

    static LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        OGL *p = reinterpret_cast<OGL *>(GetWindowLongPtr(hWnd, 0));
        switch (Msg)
        {
        case WM_KEYDOWN:
            p->Key[wParam] = true;
            break;
        case WM_KEYUP:
            p->Key[wParam] = false;
            break;
        case WM_DESTROY:
            p->Running = false;
            break;
        default:
            return DefWindowProcW(hWnd, Msg, wParam, lParam);
        }
        if ((p->Key['Q'] && p->Key[VK_CONTROL]) || p->Key[VK_ESCAPE])
        {
            p->Running = false;
        }
        return 0;
    }

    OGL() :
        Window(nullptr),
        hDC(nullptr),
        WglContext(nullptr),
        GLEContext(),
        Running(false),
        WinSizeW(0),
        WinSizeH(0),
        fboId(0),
        hInstance(nullptr)
    {
		// Clear input
		for (int i = 0; i < sizeof(Key) / sizeof(Key[0]); ++i)
            Key[i] = false;
    }

    ~OGL()
    {
        ReleaseDevice();
        CloseWindow();
    }

    bool InitWindow(HINSTANCE hInst, LPCWSTR title)
    {
        hInstance = hInst;
        Running = true;

        WNDCLASSW wc;
        memset(&wc, 0, sizeof(wc));
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = WindowProc;
        wc.cbWndExtra = sizeof(struct OGL *);
        wc.hInstance = GetModuleHandleW(NULL);
        wc.lpszClassName = L"ORT";
        RegisterClassW(&wc);

        // adjust the window size and show at InitDevice time
        Window = CreateWindowW(wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, 0, 0, hInstance, 0);
        if (!Window) return false;

        SetWindowLongPtr(Window, 0, LONG_PTR(this));

        hDC = GetDC(Window);

        return true;
    }

    void CloseWindow()
    {
        if (Window)
        {
            if (hDC)
            {
                ReleaseDC(Window, hDC);
                hDC = nullptr;
            }
            DestroyWindow(Window);
            Window = nullptr;
            UnregisterClassW(L"OGL", hInstance);
        }
    }

    // Note: currently there is no way to get GL to use the passed pLuid
    bool InitDevice(int vpW, int vpH, const LUID* /*pLuid*/, bool windowed = true)
    {
        WinSizeW = vpW;
        WinSizeH = vpH;

        RECT size = { 0, 0, vpW, vpH };
        AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, false);
        const UINT flags = SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW;
        if (!SetWindowPos(Window, nullptr, 0, 0, size.right - size.left, size.bottom - size.top, flags))
            return false;

        PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARBFunc = nullptr;
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARBFunc = nullptr;
        {
            // First create a context for the purpose of getting access to wglChoosePixelFormatARB / wglCreateContextAttribsARB.
            PIXELFORMATDESCRIPTOR pfd;
            memset(&pfd, 0, sizeof(pfd));
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
            pfd.cColorBits = 32;
            pfd.cDepthBits = 16;
            int pf = ChoosePixelFormat(hDC, &pfd);
            VALIDATE(pf, "Failed to choose pixel format.");

            VALIDATE(SetPixelFormat(hDC, pf, &pfd), "Failed to set pixel format.");

            HGLRC context = wglCreateContext(hDC);
            VALIDATE(context, "wglCreateContextfailed.");
            VALIDATE(wglMakeCurrent(hDC, context), "wglMakeCurrent failed.");

            wglChoosePixelFormatARBFunc = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARBFunc = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
            OVR_ASSERT(wglChoosePixelFormatARBFunc && wglCreateContextAttribsARBFunc);

            wglDeleteContext(context);
        }

        // Now create the real context that we will be using.
        int iAttributes[] =
        {
            // WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_COLOR_BITS_ARB, 32,
            WGL_DEPTH_BITS_ARB, 16,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0, 0
        };

        float fAttributes[] = { 0, 0 };
        int   pf = 0;
        UINT  numFormats = 0;

        VALIDATE(wglChoosePixelFormatARBFunc(hDC, iAttributes, fAttributes, 1, &pf, &numFormats),
            "wglChoosePixelFormatARBFunc failed.");

        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(pfd));
        VALIDATE(SetPixelFormat(hDC, pf, &pfd), "SetPixelFormat failed.");

        GLint attribs[16];
        int   attribCount = 0;
        if (UseDebugContext)
        {
            attribs[attribCount++] = WGL_CONTEXT_FLAGS_ARB;
            attribs[attribCount++] = WGL_CONTEXT_DEBUG_BIT_ARB;
        }

        attribs[attribCount] = 0;

        WglContext = wglCreateContextAttribsARBFunc(hDC, 0, attribs);
        VALIDATE(wglMakeCurrent(hDC, WglContext), "wglMakeCurrent failed.");

        OVR::GLEContext::SetCurrentContext(&GLEContext);
        GLEContext.Init();

        glGenFramebuffers(1, &fboId);

        glEnable(GL_DEPTH_TEST);
        glFrontFace(GL_CW);
        glEnable(GL_CULL_FACE);

        if (UseDebugContext && GLE_ARB_debug_output)
        {
            glDebugMessageCallbackARB(DebugGLCallback, NULL);
            if (glGetError())
            {
                OVR_DEBUG_LOG(("glDebugMessageCallbackARB failed."));
            }

            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

            // Explicitly disable notification severity output.
            glDebugMessageControlARB(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
        }

        return true;
    }

    bool HandleMessages(void)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return Running;
    }

    void Run(bool (*MainLoop)(bool retryCreate))
    {
        // false => just fail on any error
        VALIDATE(MainLoop(false), "Oculus Rift not detected.");
        while (HandleMessages())
        {
            // true => we'll attempt to retry for ovrError_DisplayLost
            if (!MainLoop(true))
                break;
            // Sleep a bit before retrying to reduce CPU load while the HMD is disconnected
            Sleep(10);
        }
    }

    void ReleaseDevice()
    {
        if (fboId)
        {
            glDeleteFramebuffers(1, &fboId);
            fboId = 0;
        }
        if (WglContext)
        {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(WglContext);
            WglContext = nullptr;
        }
        GLEContext.Shutdown();
    }

    static void GLAPIENTRY DebugGLCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
    {
        OVR_DEBUG_LOG(("Message from OpenGL: %s\n", message));
    }
};

// Global OpenGL state
static OGL Platform;

//------------------------------------------------------------------------------
struct ShaderFill
{
    GLuint            program;
    TextureBuffer   * texture;

    ShaderFill(GLuint vertexShader, GLuint pixelShader, TextureBuffer* _texture)
    {
        texture = _texture;

        program = glCreateProgram();

        glAttachShader(program, vertexShader);
        glAttachShader(program, pixelShader);

        glLinkProgram(program);

        glDetachShader(program, vertexShader);
        glDetachShader(program, pixelShader);

        GLint r;
        glGetProgramiv(program, GL_LINK_STATUS, &r);
        if (!r)
        {
            GLchar msg[1024];
            glGetProgramInfoLog(program, sizeof(msg), 0, msg);
            OVR_DEBUG_LOG(("Linking shaders failed: %s\n", msg));
        }
    }

    ~ShaderFill()
    {
        if (program)
        {
            glDeleteProgram(program);
            program = 0;
        }
        if (texture)
        {
            delete texture;
            texture = nullptr;
        }
    }
};

//----------------------------------------------------------------
struct VertexBuffer
{
    GLuint    buffer;

    VertexBuffer(void* vertices, size_t size)
    {
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }
    ~VertexBuffer()
    {
        if (buffer)
        {
            glDeleteBuffers(1, &buffer);
            buffer = 0;
        }
    }
};

//----------------------------------------------------------------
struct IndexBuffer
{
    GLuint    buffer;

    IndexBuffer(void* indices, size_t size)
    {
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
    }
    ~IndexBuffer()
    {
        if (buffer)
        {
            glDeleteBuffers(1, &buffer);
            buffer = 0;
        }
    }
};

//---------------------------------------------------------------------------
struct Model
{
    struct Vertex
    {
        Vector3f  Pos;
		Vector3f Normal;
        DWORD     C;
        float     U, V;
    };

    Vector3f        Pos;
    Quatf           Rot;
    Matrix4f        Mat;
	float           Scale;
    int             numVertices, numIndices;
    Vertex          Vertices[2000]; // Note fixed maximum
    GLushort        Indices[2000];
    ShaderFill    * Fill;
    VertexBuffer  * vertexBuffer;
    IndexBuffer   * indexBuffer;
	int             IsVisible;
	int             IsArrow;

    Model(Vector3f pos, ShaderFill * fill) :
        numVertices(0),
        numIndices(0),
        Pos(pos),
        Rot(),
        Mat(),
        Fill(fill),
        vertexBuffer(nullptr),
        indexBuffer(nullptr),
		Scale(1.f),
		IsVisible(0),
		IsArrow(0)
    {}

    ~Model()
    {
        FreeBuffers();
    }

    Matrix4f& GetMatrix()
    {
        Mat = Matrix4f(Rot);
		Mat = Matrix4f::Scaling(Scale) * Mat;
		Mat = Matrix4f::Translation(Pos) * Mat;
		return Mat;
    }

	void AddVertex(const Vertex& v) { Vertices[numVertices++] = v; assert(numVertices < sizeof(Vertices) / sizeof(Vertices[0])); }
	void AddIndex(GLushort a) { Indices[numIndices++] = a; assert(numVertices < sizeof(Indices) / sizeof(Indices[0]));  }

    void AllocateBuffers()
    {
        vertexBuffer = new VertexBuffer(&Vertices[0], numVertices * sizeof(Vertices[0]));
        indexBuffer = new IndexBuffer(&Indices[0], numIndices * sizeof(Indices[0]));
    }

    void FreeBuffers()
    {
        delete vertexBuffer; vertexBuffer = nullptr;
        delete indexBuffer; indexBuffer = nullptr;
    }

	void AddSolidColorBox(float x1, float y1, float z1, float x2, float y2, float z2, DWORD c)
	{
		Vector3f Vert[][2] =
		{
			Vector3f(x1, y2, z1), Vector3f(z1, x1), Vector3f(x2, y2, z1), Vector3f(z1, x2),
			Vector3f(x2, y2, z2), Vector3f(z2, x2), Vector3f(x1, y2, z2), Vector3f(z2, x1),
			Vector3f(x1, y1, z1), Vector3f(z1, x1), Vector3f(x2, y1, z1), Vector3f(z1, x2),
			Vector3f(x2, y1, z2), Vector3f(z2, x2), Vector3f(x1, y1, z2), Vector3f(z2, x1),
			Vector3f(x1, y1, z2), Vector3f(z2, y1), Vector3f(x1, y1, z1), Vector3f(z1, y1),
			Vector3f(x1, y2, z1), Vector3f(z1, y2), Vector3f(x1, y2, z2), Vector3f(z2, y2),
			Vector3f(x2, y1, z2), Vector3f(z2, y1), Vector3f(x2, y1, z1), Vector3f(z1, y1),
			Vector3f(x2, y2, z1), Vector3f(z1, y2), Vector3f(x2, y2, z2), Vector3f(z2, y2),
			Vector3f(x1, y1, z1), Vector3f(x1, y1), Vector3f(x2, y1, z1), Vector3f(x2, y1),
			Vector3f(x2, y2, z1), Vector3f(x2, y2), Vector3f(x1, y2, z1), Vector3f(x1, y2),
			Vector3f(x1, y1, z2), Vector3f(x1, y1), Vector3f(x2, y1, z2), Vector3f(x2, y1),
			Vector3f(x2, y2, z2), Vector3f(x2, y2), Vector3f(x1, y2, z2), Vector3f(x1, y2)
		};

		GLushort CubeIndices[] =
		{
			0, 1, 3, 3, 1, 2,
			5, 4, 6, 6, 4, 7,
			8, 9, 11, 11, 9, 10,
			13, 12, 14, 14, 12, 15,
			16, 17, 19, 19, 17, 18,
			21, 20, 22, 22, 20, 23
		};

		for (int i = 0; i < sizeof(CubeIndices) / sizeof(CubeIndices[0]); ++i)
			AddIndex(CubeIndices[i] + GLushort(numVertices));

		// Generate a quad for each box face
		for (int v = 0; v < 6 * 4; v++)
		{
			// Make vertices, with some token lighting
			Vertex vvv; vvv.Pos = Vert[v][0]; vvv.U = Vert[v][1].x; vvv.V = Vert[v][1].y;
			float dist1 = (vvv.Pos - Vector3f(-2, 4, -2)).Length();
			float dist2 = (vvv.Pos - Vector3f(3, 4, -3)).Length();
			float dist3 = (vvv.Pos - Vector3f(-4, 3, 25)).Length();
			int   bri = rand() % 160;
			float B = ((c >> 16) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
			float G = ((c >> 8) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
			float R = ((c >> 0) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
			vvv.C = (c & 0xff000000) +
				((R > 255 ? 255 : DWORD(R)) << 16) +
				((G > 255 ? 255 : DWORD(G)) << 8) +
				(B > 255 ? 255 : DWORD(B));
			AddVertex(vvv);
		}
	}

	void AddColorPyramid(float x1, float y1, float z1, float x2, float y2, float z2, DWORD c)
	{
		Vector3f Vert[][2] =
		{
			Vector3f(x1, y2, z1), Vector3f(z1, x1), Vector3f(x2, y2, z1), Vector3f(z1, x2),
			Vector3f(x2, y2, z2), Vector3f(z2, x2), Vector3f(x1, y2, z2), Vector3f(z2, x1),
			Vector3f(x1, y1, z1), Vector3f(z1, x1), Vector3f(x2, y1, z1), Vector3f(z1, x2),
			Vector3f(x2, y1, z2), Vector3f(z2, x2), Vector3f(x1, y1, z2), Vector3f(z2, x1),
			Vector3f(x1, y1, z2), Vector3f(z2, y1), Vector3f(x1, y1, z1), Vector3f(z1, y1),
			Vector3f(x1, y2, z1), Vector3f(z1, y2), Vector3f(x1, y2, z2), Vector3f(z2, y2),
			Vector3f(x2, y1, z2), Vector3f(z2, y1), Vector3f(x2, y1, z1), Vector3f(z1, y1),
			Vector3f(x2, y2, z1), Vector3f(z1, y2), Vector3f(x2, y2, z2), Vector3f(z2, y2),
			Vector3f(x1, y1, z1), Vector3f(x1, y1), Vector3f(x2, y1, z1), Vector3f(x2, y1),
			Vector3f(x2, y2, z1), Vector3f(x2, y2), Vector3f(x1, y2, z1), Vector3f(x1, y2),
			Vector3f(x1, y1, z2), Vector3f(x1, y1), Vector3f(x2, y1, z2), Vector3f(x2, y1),
			Vector3f(x2, y2, z2), Vector3f(x2, y2), Vector3f(x1, y2, z2), Vector3f(x1, y2)
		};

		// This is the ZJD hack to squish the xy when z is at one side (z2)
		for (int v = 0; v < 6 * 4; v++) {
			if (Vert[v][0].z == z2) {
				Vert[v][0].x = (x2 + x1) / 2.f;
				Vert[v][0].y = (y2 + y1) / 2.f;

				//  Somehow we need to modify the uv
				//Vert[v][1].x = (x2 + x1) / 2.f;
				//Vert[v][1].y = (y2 + y1) / 2.f;
				//Vert[v][1].x = 0.f;
				//Vert[v][1].y = 0.f;
			}
		}

		GLushort CubeIndices[] =
		{
			0, 1, 3, 3, 1, 2,
			5, 4, 6, 6, 4, 7,
			8, 9, 11, 11, 9, 10,
			13, 12, 14, 14, 12, 15,
			16, 17, 19, 19, 17, 18,
			21, 20, 22, 22, 20, 23
		};

		for (int i = 0; i < sizeof(CubeIndices) / sizeof(CubeIndices[0]); ++i)
			AddIndex(CubeIndices[i] + GLushort(numVertices));

		// Generate a quad for each box face
		for (int v = 0; v < 6 * 4; v++)
		{
			// Make vertices, with some token lighting
			Vertex vvv; vvv.Pos = Vert[v][0]; vvv.U = Vert[v][1].x; vvv.V = Vert[v][1].y;
			float dist1 = (vvv.Pos - Vector3f(-2, 4, -2)).Length();
			float dist2 = (vvv.Pos - Vector3f(3, 4, -3)).Length();
			float dist3 = (vvv.Pos - Vector3f(-4, 3, 25)).Length();
			int   bri = rand() % 160;
			float B = ((c >> 16) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
			float G = ((c >> 8) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
			float R = ((c >> 0) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
			vvv.C = (c & 0xff000000) +
				((R > 255 ? 255 : DWORD(R)) << 16) +
				((G > 255 ? 255 : DWORD(G)) << 8) +
				(B > 255 ? 255 : DWORD(B));
			AddVertex(vvv);
		}
	}

	void AddArrow() {
		const float blackU = 0.f;
		const float blackV = 0.f;
		const float whiteU = 0.5f;
		const float whiteV = 0.5f;

		int rot = 16;
		const float PI2F = 2.f * 3.14159f;
		for (int i = 0; i < rot; i++) {
			float t0 = PI2F * (float)(i + 0) / (float)rot;
			float t1 = PI2F * (float)(i + 1) / (float)rot;
			float c0 = cosf(t0);
			float s0 = sinf(t0);
			float c1 = cosf(t1);
			float s1 = sinf(t1);
			float tr = 0.1f;
			float hr = 0.2f;

			// Triangle 0
			Vertex v;

			v.Pos = Vector3f(0.f, 0.f, 0.f);
			v.Normal = Vector3f(0.f, 0.f, -1.f);
			v.C = 0xffffffff;
			v.U = blackU;
			v.V = blackV;
			AddVertex(v);

			v.Pos = Vector3f(tr*c0, tr*s0, 0.f);
			v.Normal = Vector3f(0.f, 0.f, -1.f);
			v.C = 0xffffffff;
			v.U = blackU;
			v.V = blackV;
			AddVertex(v);

			v.Pos = Vector3f(tr*c1, tr*s1, 0.f);
			v.Normal = Vector3f(0.f, 0.f, -1.f);
			v.C = 0xffffffff;
			v.U = blackU;
			v.V = blackV;
			AddVertex(v);

			// Triangle 1
			v.Pos = Vector3f(tr*c0, tr*s0, 0.5f);
			v.Normal = Vector3f(c0, s0, 0.f);
			v.C = 0xffffffff;
			v.U = blackU;
			v.V = blackV;
			AddVertex(v);

			v.Pos = Vector3f(tr*c1, tr*s1, 0.f);
			v.Normal = Vector3f(c0, s0, 0.f);
			v.C = 0xffffffff;
			v.U = blackU;
			v.V = blackV;
			AddVertex(v);

			v.Pos = Vector3f(tr*c0, tr*s0, 0.f);
			v.Normal = Vector3f(c0, s0, 0.f);
			v.C = 0xffffffff;
			v.U = blackU;
			v.V = blackV;
			AddVertex(v);

			// Triangle 2
			v.Pos = Vector3f(tr*c0, tr*s0, 0.5f);
			v.Normal = Vector3f(c0, s0, 0.f);
			v.C = 0xffffffff;
			v.U = blackU;
			v.V = blackV;
			AddVertex(v);

			v.Pos = Vector3f(tr*c1, tr*s1, 0.5f);
			v.Normal = Vector3f(c0, s0, 0.f);
			v.C = 0xffffffff;
			v.U = blackU;
			v.V = blackV;
			AddVertex(v);

			v.Pos = Vector3f(tr*c1, tr*s1, 0.f);
			v.Normal = Vector3f(c0, s0, 0.f);
			v.C = 0xffffffff;
			v.U = blackU;
			v.V = blackV;
			AddVertex(v);

			// Triangle 3
			v.Pos = Vector3f(hr*c0, hr*s0, 0.5f);
			v.Normal = Vector3f(0.f, 0.f, -1.f);
			v.C = 0xffffffff;
			v.U = whiteU;
			v.V = whiteV;
			AddVertex(v);

			v.Pos = Vector3f(tr*c1, tr*s1, 0.5f);
			v.Normal = Vector3f(0.f, 0.f, -1.f);
			v.C = 0xffffffff;
			v.U = whiteU;
			v.V = whiteV;
			AddVertex(v);

			v.Pos = Vector3f(tr*c0, tr*s0, 0.5f);
			v.Normal = Vector3f(0.f, 0.f, -1.f);
			v.C = 0xffffffff;
			v.U = whiteU;
			v.V = whiteV;
			AddVertex(v);

			// Triangle 4
			v.Pos = Vector3f(hr*c1, hr*s1, 0.5f);
			v.Normal = Vector3f(0.f, 0.f, -1.f);
			v.C = 0xffffffff;
			v.U = whiteU;
			v.V = whiteV;
			AddVertex(v);

			v.Pos = Vector3f(tr*c1, tr*s1, 0.5f);
			v.Normal = Vector3f(0.f, 0.f, -1.f);
			v.C = 0xffffffff;
			v.U = whiteU;
			v.V = whiteV;
			AddVertex(v);

			v.Pos = Vector3f(hr*c0, hr*s0, 0.5f);
			v.Normal = Vector3f(0.f, 0.f, -1.f);
			v.C = 0xffffffff;
			v.U = whiteU;
			v.V = whiteV;
			AddVertex(v);

			// Triangle 5
			Vector3f a(0.f, 0.f, 1.f);
			Vector3f b(hr*c1, hr*s1, 0.5f);
			Vector3f c(hr*c0, hr*s0, 0.5f);
			Vector3f ca = c - a;
			Vector3f n = ca.Cross(b - a);
			n.Normalize();
			v.Pos = a;
			v.Normal = n;
			v.C = 0xffffffff;
			v.U = whiteU;
			v.V = whiteV;
			AddVertex(v);

			v.Pos = b;
			v.Normal = n;
			v.C = 0xffffffff;
			v.U = whiteU;
			v.V = whiteV;
			AddVertex(v);

			v.Pos = c;
			v.Normal = n;
			v.C = 0xffffffff;
			v.U = whiteU;
			v.V = whiteV;
			AddVertex(v);
		}

		for (int i = 0; i < numVertices; i+=3) {
			AddIndex(i+0);
			AddIndex(i+1);
			AddIndex(i+2);
		}
	}

	void AddArrow1() {
		/*Vertex vvv;
		vvv.Pos = Vector3f( 0.f, 0.f, 0.f );
		vvv.Normal = Vector3f( 0.f, 0.f, -1.f );
		vvv.U = 0.5f;
		vvv.V = 0.5f;
		vvv.C = 0xffffffff;
		AddVertex(vvv);

		vvv.Pos = Vector3f(0.f, 1.f, 0.f);
		vvv.Normal = Vector3f(0.f, 0.f, -1.f);
		vvv.U = 0.5f;
		vvv.V = 0.5f;
		vvv.C = 0xffffffff;
		AddVertex(vvv);

		vvv.Pos = Vector3f(1.f, 1.f, 0.f);
		vvv.Normal = Vector3f(0.f, 0.f, -1.f);
		vvv.U = 0.5f;
		vvv.V = 0.5f;
		vvv.C = 0xffffffff;
		AddVertex(vvv);

		AddIndex(2);
		AddIndex(1);
		AddIndex(0);

		return;
		*/
		float verts[1024][3] = {
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f }
		};

		const float blackU = 0.f;
		const float blackV = 0.f;
		const float whiteU = 0.5f;
		const float whiteV = 0.5f;

		float uv[1024][2] = {
			{ blackU, blackV },
			{ whiteU, whiteV }
		};


		int rot = 8;
		int v = 2;
		const float PI2F = 2.f * 3.14159f;
		for (int i = 0; i<rot; i++) {
			float t0 = PI2F * ((float)(i + 0) / (float)rot);
			float t1 = PI2F * ((float)(i + 1) / (float)rot);
			verts[v][0] = 0.1f * cosf(t0);
			verts[v][1] = 0.1f * sinf(t0);
			verts[v][2] = 0.0f;
			uv[v][0] = blackU;
			uv[v][1] = blackV;
			v++;

			verts[v][0] = 0.1f * cosf(t0);
			verts[v][1] = 0.1f * sinf(t0);
			verts[v][2] = 0.5f;
			uv[v][0] = blackU;
			uv[v][1] = blackV;
			v++;

			verts[v][0] = 0.2f * cosf(t0);
			verts[v][1] = 0.2f * sinf(t0);
			verts[v][2] = 0.5f;
			uv[v][0] = whiteU;
			uv[v][1] = whiteV;
			v++;

			verts[v][0] = 0.1f * cosf(t1);
			verts[v][1] = 0.1f * sinf(t1);
			verts[v][2] = 0.0f;
			uv[v][0] = blackU;
			uv[v][1] = blackV;
			v++;

			verts[v][0] = 0.1f * cosf(t1);
			verts[v][1] = 0.1f * sinf(t1);
			verts[v][2] = 0.5f;
			uv[v][0] = blackU;
			uv[v][1] = blackV;
			v++;

			verts[v][0] = 0.2f * cosf(t1);
			verts[v][1] = 0.2f * sinf(t1);
			verts[v][2] = 0.5f;
			uv[v][0] = whiteU;
			uv[v][1] = whiteV;
			v++;
		}

		for (int i = 0; i < v; i++) {
			Vertex vvv;
			vvv.Pos = Vector3f(verts[i][0], verts[i][1], verts[i][2]);
			vvv.C = 0xffffffff;
			vvv.U = uv[i][0];
			vvv.V = uv[i][1];
			AddVertex(vvv);
		}

		unsigned int indicies[1024];

		int index = 0;
		for (int i = 0; i<rot; i++) {
			int q = i * 6;

			indicies[index++] = q + 2;
			indicies[index++] = q + 5;
			indicies[index++] = 0;

			indicies[index++] = q + 3;
			indicies[index++] = q + 5;
			indicies[index++] = q + 2;

			indicies[index++] = q + 3;
			indicies[index++] = q + 6;
			indicies[index++] = q + 5;

			indicies[index++] = q + 4;
			indicies[index++] = q + 6;
			indicies[index++] = q + 3;

			indicies[index++] = q + 7;
			indicies[index++] = q + 6;
			indicies[index++] = q + 4;

			indicies[index++] = 1;
			indicies[index++] = q + 7;
			indicies[index++] = q + 4;
		}

		for (int i = 0; i < index; i++) {
			AddIndex(indicies[i]);
		}
	}



	void Render(Matrix4f view, Matrix4f proj)
    {
        Matrix4f combined = proj * view * GetMatrix();

		Matrix4f viewOnly = GetMatrix();

        glUseProgram(Fill->program);
        glUniform1i(glGetUniformLocation(Fill->program, "Texture0"), 0);
        glUniformMatrix4fv(glGetUniformLocation(Fill->program, "matWVP"), 1, GL_TRUE, (FLOAT*)&combined);
		glUniformMatrix4fv(glGetUniformLocation(Fill->program, "matWV"), 1, GL_TRUE, (FLOAT*)&viewOnly);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Fill->texture->texId);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->buffer);

        GLuint posLoc = glGetAttribLocation(Fill->program, "Position");
        GLuint colorLoc = glGetAttribLocation(Fill->program, "Color");
        GLuint uvLoc = glGetAttribLocation(Fill->program, "TexCoord");
		GLuint normalLoc = glGetAttribLocation(Fill->program, "Normal");

        glEnableVertexAttribArray(posLoc);
        glEnableVertexAttribArray(colorLoc);
        glEnableVertexAttribArray(uvLoc);
		glEnableVertexAttribArray(normalLoc);

        glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, Pos));
        glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, C));
        glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, U));
		glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, Normal));

        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, NULL);

        glDisableVertexAttribArray(posLoc);
        glDisableVertexAttribArray(colorLoc);
        glDisableVertexAttribArray(uvLoc);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glUseProgram(0);
    }
};

//------------------------------------------------------------------------- 
struct Scene
{
    int     numModels;
    Model * Models[5000];
	const int maxArrows = 100;

    void Add(Model * n)
    {
		assert(numModels < sizeof(Models) / sizeof(Models[0]) );
        Models[numModels++] = n;
    }

	float randf() {
		return (float)(rand() % 1000) / 1000.f - 0.5f;
	}

    void Render(Matrix4f view, Matrix4f proj)
    {
		Vector3f cen( 0, 0, 0 );
		const int numChg = 2;
		Vector3f chgPos[numChg];
		chgPos[0] = cen - Vector3f(-2.f, 0, 0);
		chgPos[1] = cen - Vector3f(+2.f, 0, 0);
		float chg[2] = { -1.f, +1.f };
		Vector3f z(0.f, 0.f, 1.f);

		for (int i = 0; i < numModels; ++i) {
			if (Models[i]->IsArrow) {
				if (Models[i]->IsVisible) {
					// INTEGRATE along f
					Vector3f xyz = Models[i]->Pos;
					Vector3f f;//(0.f, 1.f, 0.f);
					
					for (int j = 0; j < numChg; j++) {
						Vector3f r = xyz - chgPos[j];
						float mag = chg[j] / r.LengthSq();
						r.Normalize();
						f += r * mag;
					}

					Models[i]->Pos += f * 0.01f;
					Models[i]->Scale = f.Length();
					f.Normalize();
					Models[i]->Rot = Quatf::Align(f, z);
					Models[i]->Render(view, proj);

					Vector3f rToChg0 = xyz - chgPos[0];
					if (rToChg0.Length() < 1.f) {
						Models[i]->IsVisible = 0;
					}

					if (xyz.Length() > 6.f) {
						Models[i]->IsVisible = 0;
					}
				}
				else {
					if (rand() % 1 == 0) {
						Models[i]->IsVisible = 1;
						Models[i]->Pos = Vector3f(randf(), randf(), randf());
						Models[i]->Pos.Normalize();
						Models[i]->Pos *= 0.1f;
						Models[i]->Pos += chgPos[1];
					}
				}
			}
			else {
				Models[i]->Render(view, proj);
			}
		}
    }

    GLuint CreateShader(GLenum type, const GLchar* src)
    {
        GLuint shader = glCreateShader(type);

        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);

        GLint r;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &r);
        if (!r)
        {
            GLchar msg[1024];
            glGetShaderInfoLog(shader, sizeof(msg), 0, msg);
            if (msg[0]) {
                OVR_DEBUG_LOG(("Compiling shader failed: %s\n", msg));
            }
            return 0;
        }

        return shader;
    }

    void Init(int includeIntensiveGPUobject)
    {
		static const GLchar* VertexShaderSrc =
			"#version 150\n"
			"uniform mat4 matWVP;\n"
			"uniform mat4 matWV;\n"
			"in      vec4 Position;\n"
			"in      vec4 Color;\n"
			"in      vec2 TexCoord;\n"
			"in      vec3 Normal;\n"
			"out     vec2 oTexCoord;\n"
			"out     vec4 oColor;\n"
			"void main()\n"
			"{\n"
			"	vec4 b = vec4(Normal.x, Normal.y, Normal.z, 0.0);\n"
			"	vec4 n = (matWV * b);\n"
			"	float nDotVP = max(0.0, dot(n, vec4(1.414213562373095, 1.414213562373095, 0.0, 1.0)));\n"
			"	if(length(Normal)==0.0) { nDotVP = 1; }\n"
			"   gl_Position = (matWVP * Position);\n"
            "   oTexCoord   = TexCoord;\n"
			"   oColor.rgb  = pow(Color.rgb, vec3(2.2)) * nDotVP + vec3(0.06);\n"   // convert from sRGB to linear
//			"   oColor.rgb  = Color.rgb * nDotVP + vec3(0.06);\n"   // convert from sRGB to linear
			"   oColor.a    = Color.a;\n"
            "}\n";

        static const char* FragmentShaderSrc =
            "#version 150\n"
            "uniform sampler2D Texture0;\n"
            "in      vec4      oColor;\n"
            "in      vec2      oTexCoord;\n"
            "out     vec4      FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = oColor * texture2D(Texture0, oTexCoord);\n"
            "}\n";

        GLuint    vshader = CreateShader(GL_VERTEX_SHADER, VertexShaderSrc);
        GLuint    fshader = CreateShader(GL_FRAGMENT_SHADER, FragmentShaderSrc);

        // Make textures
        ShaderFill * grid_material[5];
        for (int k = 0; k < 5; ++k)
        {
            static DWORD tex_pixels[256 * 256];
            for (int j = 0; j < 256; ++j)
            {
                for (int i = 0; i < 256; ++i)
                {
                    if (k == 0) tex_pixels[j * 256 + i] = (((i >> 7) ^ (j >> 7)) & 1) ? 0xffb4b4b4 : 0xff505050;// floor
                    if (k == 1) tex_pixels[j * 256 + i] = (((j / 4 & 15) == 0) || (((i / 4 & 15) == 0) && ((((i / 4 & 31) == 0) ^ ((j / 4 >> 4) & 1)) == 0)))
                        ? 0xff3c3c3c : 0xffb4b4b4;// wall
                    if (k == 2) tex_pixels[j * 256 + i] = (i / 4 == 0 || j / 4 == 0) ? 0xff505050 : 0xffb4b4b4;// ceiling
					if (k == 3) tex_pixels[j * 256 + i] = 0xffffffff;// blank
					if (k == 4) {
						tex_pixels[j * 256 + i] = (j == 255 || j == 0 || i == 255 || i == 0) ? 0xffffffff : 0xffff0000;
					}
				}
            }
            TextureBuffer * generated_texture = new TextureBuffer(nullptr, false, false, Sizei(256, 256), 4, (unsigned char *)tex_pixels, 1);
            grid_material[k] = new ShaderFill(vshader, fshader, generated_texture);
        }

        glDeleteShader(vshader);
        glDeleteShader(fshader);

		Model *m;

		for (int i = 0; i < maxArrows; i++ ) {
			m = new Model(Vector3f(0, 0, 0), grid_material[4]);
			m->AddArrow();
			m->AllocateBuffers();
			m->IsVisible = 0;
			m->IsArrow = 1;
			Add(m);
		}

		float x1 = -10.f;
		float x2 = +10.f;
		float y1 = -10.f;
		float y2 = +10.f;
		float z1 = -10.f;
		float z2 = +10.f;

		m = new Model(Vector3f(0, 0, 0), grid_material[1]);  // Walls
        m->AddSolidColorBox(x1, y1, z1, x1-0.1f, y2, z2, 0xff808080); // Right Wall
		m->AddSolidColorBox(x2, y1, z1, x2+0.1f, y2, z2, 0xff808080); // Left Wall
		m->AddSolidColorBox(x1, y1, z1, x2, y2, z1-0.1f, 0xff808080); // Front Wall
		m->AddSolidColorBox(x1, y1, z2, x2, y2, z2+0.1f, 0xff808080); // Back Wall
		m->AllocateBuffers();
        Add(m);

        m = new Model(Vector3f(0, 0, 0), grid_material[0]);  // Floors
        m->AddSolidColorBox(x1, y1, z1, x2, y1-0.1f, z2, 0xff808080); // Main floor
        m->AllocateBuffers();
        Add(m);

        m = new Model(Vector3f(0, 0, 0), grid_material[2]);  // Ceiling
        m->AddSolidColorBox(x1, y2, z1, x2, y2+0.1f, z2, 0xff808080);
        m->AllocateBuffers();
        Add(m);
    }

    Scene() : numModels(0) {}
    Scene(bool includeIntensiveGPUobject) :
        numModels(0)
    {
        Init(includeIntensiveGPUobject);
    }
    void Release()
    {
        while (numModels-- > 0)
            delete Models[numModels];
    }
    ~Scene()
    {
        Release();
    }
};
