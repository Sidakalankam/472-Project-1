#pragma once

#include "graphics/GrRenderer.h"

class CMyRaytraceRenderer : public CGrRenderer
{
public:
    CMyRaytraceRenderer();

    // Set image buffer
    void SetImage(BYTE** image, int width, int height);

    // Set window (for updates)
    void SetWindow(CWnd* p_window);

private:
    int     m_rayimagewidth;
    int     m_rayimageheight;
    BYTE** m_rayimage;

    CWnd* m_window;
};