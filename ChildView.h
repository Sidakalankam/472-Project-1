#pragma once

#include "graphics/OpenGLWnd.h"
#include "graphics/GrCamera.h"
#include "graphics/GrObject.h"
#include "CMyRaytraceRenderer.h"

class CChildView : public COpenGLWnd
{
public:
    CChildView();

public:
    CGrCamera m_camera;
    CGrPtr<CGrObject> m_scene;

    bool m_raytrace;

    BYTE** m_rayimage;
    int         m_rayimagewidth;
    int         m_rayimageheight;

public:
    virtual void OnGLDraw(CDC* pDC);
    void ConfigureRenderer(CGrRenderer* p_renderer);

protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:
    virtual ~CChildView();

protected:
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);

    afx_msg void OnRenderRaytrace();
    afx_msg void OnUpdateRenderRaytrace(CCmdUI* pCmdUI);

    DECLARE_MESSAGE_MAP()
};