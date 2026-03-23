#pragma once

#include <list>
#include "graphics/GrRenderer.h"
#include "graphics/GrTransform.h"
#include "graphics/RayIntersection.h"

class CMyRaytraceRenderer : public CGrRenderer
{
public:
    CMyRaytraceRenderer();
    virtual bool RendererStart();
    virtual bool RendererEnd();
    virtual void RendererEndPolygon();
    virtual void RendererMaterial(CGrMaterial* p_material);
    virtual void RendererPushMatrix();
    virtual void RendererPopMatrix();
    virtual void RendererRotate(double angle, double x, double y, double z);
    virtual void RendererTranslate(double x, double y, double z);

    // Set image buffer
    void SetImage(BYTE** image, int width, int height);

    // Set window (for updates)
    void SetWindow(CWnd* p_window);

private:
    int     m_rayimagewidth;
    int     m_rayimageheight;
    BYTE** m_rayimage;

    CWnd* m_window;
    CRayIntersection m_intersection;
    std::list<CGrTransform> m_mstack;
    CGrMaterial* m_material;
};
