// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "Project 1.h"
#include "ChildView.h"

#include "graphics/GrObject.h"
#include "graphics/GrTransform.h"
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

    // Red box
    CGrPtr<CGrMaterial> redpaint = new CGrMaterial;
    redpaint->AmbientAndDiffuse(0.8f, 0.0f, 0.0f);
    scene->Child(redpaint);

    CGrPtr<CGrComposite> redbox = new CGrComposite;
    redpaint->Child(redbox);
    redbox->Box(1, 1, 1, 5, 5, 5);

    // White box
    CGrPtr<CGrMaterial> whitepaint = new CGrMaterial;
    whitepaint->AmbientAndDiffuse(0.8f, 0.8f, 0.8f);
    scene->Child(whitepaint);

    CGrPtr<CGrComposite> whitebox = new CGrComposite;
    whitepaint->Child(whitebox);
    whitebox->Box(-10, -10, -10, 5, 5, 5);
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

    float dimd = 0.5f;
    GLfloat dim[] = { dimd, dimd, dimd, 1.0f };
    GLfloat brightwhite[] = { 1.f, 1.f, 1.f, 1.0f };

    p_renderer->AddLight(
        CGrPoint(1, 0.5, 1.2, 0),
        dim,
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