// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "Project 1.h"
#include "ChildView.h"

#include "graphics/GrObject.h"
#include "graphics/GrTransform.h"
#include "graphics/GrTexture.h"
#include "graphics/OpenGLRenderer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildView

CChildView::CChildView()
{
    m_raytrace = false;

    // ? initialize image pointer
    m_rayimage = NULL;

    // Camera
    m_camera.Set(20, 10, 50, 0, 0, 0, 0, 1, 0);

    // Scene graph setup
    CGrPtr<CGrComposite> scene = new CGrComposite;
    m_scene = scene;

    // Procedural checkerboard texture used by a mapped floor
    CGrPtr<CGrTexture> checker = new CGrTexture;
    checker->SetSize(256, 256);
    for (int y = 0; y < 256; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            const bool dark = ((x / 32) + (y / 32)) % 2 == 0;
            checker->Set(x, y, dark ? 30 : 220, dark ? 30 : 220, dark ? 30 : 220);
        }
    }

    // Red box
    CGrPtr<CGrMaterial> redpaint = new CGrMaterial;
    redpaint->AmbientAndDiffuse(0.8f, 0.0f, 0.0f);
    redpaint->Specular(0.6f, 0.6f, 0.6f);
    redpaint->SpecularOther(0.2f, 0.2f, 0.2f);
    redpaint->Shininess(30.0f);
    scene->Child(redpaint);

    CGrPtr<CGrComposite> redbox = new CGrComposite;
    redpaint->Child(redbox);
    redbox->Box(1, 1, 1, 5, 5, 5);

    // White box
    CGrPtr<CGrMaterial> whitepaint = new CGrMaterial;
    whitepaint->AmbientAndDiffuse(0.8f, 0.8f, 0.8f);
    whitepaint->Specular(0.8f, 0.8f, 0.8f);
    whitepaint->SpecularOther(0.55f, 0.55f, 0.55f);
    whitepaint->Shininess(80.0f);
    scene->Child(whitepaint);

    CGrPtr<CGrComposite> whitebox = new CGrComposite;
    whitepaint->Child(whitebox);
    whitebox->Box(-10, -10, -10, 5, 5, 5);

    // Textured floor (mapped rectangle rotated into XZ plane)
    CGrPtr<CGrMaterial> floorpaint = new CGrMaterial;
    floorpaint->Standard(CGrMaterial::texture);
    floorpaint->Specular(0.2f, 0.2f, 0.2f);
    floorpaint->Shininess(8.0f);
    scene->Child(floorpaint);

    CGrPtr<CGrTranslate> floortranslate = new CGrTranslate(0, -12, 0);
    floorpaint->Child(floortranslate);

    CGrPtr<CGrRotate> floorrotate = new CGrRotate(-90, 1, 0, 0);
    floortranslate->Child(floorrotate);

    CGrPtr<CGrComposite> floorgeo = new CGrComposite;
    floorrotate->Child(floorgeo);
    floorgeo->AddMappedRect(checker, -30, -30, 30, 30, 6, 6, 0, 0);

    // Additional non-box shape: pyramid
    CGrPtr<CGrMaterial> pyramidpaint = new CGrMaterial;
    pyramidpaint->AmbientAndDiffuse(0.2f, 0.8f, 0.3f);
    pyramidpaint->Specular(0.5f, 0.5f, 0.5f);
    pyramidpaint->Shininess(24.0f);
    scene->Child(pyramidpaint);

    CGrPtr<CGrComposite> pyramid = new CGrComposite;
    pyramidpaint->Child(pyramid);
    CGrPoint p0(-18, -12, -5);
    CGrPoint p1(-12, -12, -5);
    CGrPoint p2(-12, -12, 1);
    CGrPoint p3(-18, -12, 1);
    CGrPoint apex(-15, -5, -2);
    pyramid->Poly4(p0, p3, p2, p1);
    pyramid->Poly3(p0, p1, apex);
    pyramid->Poly3(p1, p2, apex);
    pyramid->Poly3(p2, p3, apex);
    pyramid->Poly3(p3, p0, apex);

    // Additional non-box shape: tetrahedron
    CGrPtr<CGrMaterial> tetpaint = new CGrMaterial;
    tetpaint->AmbientAndDiffuse(0.2f, 0.4f, 0.9f);
    tetpaint->Specular(0.6f, 0.6f, 0.6f);
    tetpaint->Shininess(40.0f);
    scene->Child(tetpaint);

    CGrPtr<CGrComposite> tetra = new CGrComposite;
    tetpaint->Child(tetra);
    CGrPoint t0(10, -12, -14);
    CGrPoint t1(16, -12, -14);
    CGrPoint t2(13, -12, -8);
    CGrPoint t3(13, -5, -11);
    tetra->Poly3(t0, t2, t1);
    tetra->Poly3(t0, t1, t3);
    tetra->Poly3(t1, t2, t3);
    tetra->Poly3(t2, t0, t3);
}

CChildView::~CChildView()
{
    // ? CLEANUP
    if (m_rayimage != NULL)
    {
        delete[] m_rayimage[0];
        delete[] m_rayimage;
    }
}

BEGIN_MESSAGE_MAP(CChildView, COpenGLWnd)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_RBUTTONDOWN()

    ON_COMMAND(ID_RENDER_RAYTRACE, &CChildView::OnRenderRaytrace)
    ON_UPDATE_COMMAND_UI(ID_RENDER_RAYTRACE, &CChildView::OnUpdateRenderRaytrace)
END_MESSAGE_MAP()

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!COpenGLWnd::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle |= WS_EX_CLIENTEDGE;
    cs.style &= ~WS_BORDER;
    cs.lpszClass = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        ::LoadCursor(nullptr, IDC_ARROW),
        reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
        nullptr);

    return TRUE;
}

void CChildView::OnGLDraw(CDC* pDC)
{
    if (m_raytrace)
    {
        // Clear the color buffer
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set up for parallel projection
        int width, height;
        GetSize(width, height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, 0, height, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Draw the image
        if (m_rayimage)
        {
            glRasterPos3i(0, 0, 0);
            glDrawPixels(m_rayimagewidth, m_rayimageheight,
                GL_RGB, GL_UNSIGNED_BYTE, m_rayimage[0]);
        }

        glFlush();
    }
    else
    {
        // OpenGL renderer
        COpenGLRenderer renderer;
        ConfigureRenderer(&renderer);
        renderer.Render(m_scene);
    }
}

void CChildView::ConfigureRenderer(CGrRenderer* p_renderer)
{
    int width, height;
    GetSize(width, height);
    double aspectratio = double(width) / double(height);

    p_renderer->Perspective(
        m_camera.FieldOfView(),
        aspectratio,
        20.,
        1000.);

    p_renderer->LookAt(
        m_camera.Eye()[0], m_camera.Eye()[1], m_camera.Eye()[2],
        m_camera.Center()[0], m_camera.Center()[1], m_camera.Center()[2],
        m_camera.Up()[0], m_camera.Up()[1], m_camera.Up()[2]);

    GLfloat lowambient[] = { 0.15f, 0.15f, 0.15f, 1.0f };
    GLfloat softwhite[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat brightwhite[] = { 1.f, 1.f, 1.f, 1.0f };

    p_renderer->AddLight(
        CGrPoint(1, 0.5, 1.2, 0),
        lowambient,
        softwhite,
        softwhite);

    p_renderer->AddLight(
        CGrPoint(20, 18, 25, 1),
        lowambient,
        brightwhite,
        brightwhite);
}

//
// ? MAIN NEW CODE (image allocation)
//
void CChildView::OnRenderRaytrace()
{
    m_raytrace = !m_raytrace;
    Invalidate();

    if (!m_raytrace)
        return;

    // Delete old image
    if (m_rayimage != NULL)
    {
        delete[] m_rayimage[0];
        delete[] m_rayimage;
        m_rayimage = NULL;
    }

    // Get size
    GetSize(m_rayimagewidth, m_rayimageheight);

    // Allocate rows
    m_rayimage = new BYTE * [m_rayimageheight];

    int rowwid = m_rayimagewidth * 3;
    while (rowwid % 4)
        rowwid++;

    m_rayimage[0] = new BYTE[m_rayimageheight * rowwid];

    for (int i = 1; i < m_rayimageheight; i++)
    {
        m_rayimage[i] = m_rayimage[0] + i * rowwid;
    }

    // Fill blue
    for (int i = 0; i < m_rayimageheight; i++)
    {
        for (int j = 0; j < m_rayimagewidth; j++)
        {
            m_rayimage[i][j * 3] = 0;
            m_rayimage[i][j * 3 + 1] = 0;
            m_rayimage[i][j * 3 + 2] = BYTE(255);
        }
    }

    CMyRaytraceRenderer raytrace;

    ConfigureRenderer(&raytrace);

    raytrace.SetImage(m_rayimage, m_rayimagewidth, m_rayimageheight);

    raytrace.SetWindow(this);

    raytrace.Render(m_scene);

    Invalidate();
}

void CChildView::OnUpdateRenderRaytrace(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_raytrace);
}

// Mouse handlers

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_camera.MouseDown(point.x, point.y);
    COpenGLWnd::OnLButtonDown(nFlags, point);
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_camera.MouseMove(point.x, point.y, nFlags))
        Invalidate();

    COpenGLWnd::OnMouseMove(nFlags, point);
}

void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
    m_camera.MouseDown(point.x, point.y, 2);
    COpenGLWnd::OnRButtonDown(nFlags, point);
}
