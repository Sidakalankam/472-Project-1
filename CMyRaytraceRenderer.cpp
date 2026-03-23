#include "pch.h"
#include "CMyRaytraceRenderer.h"

CMyRaytraceRenderer::CMyRaytraceRenderer()
{
    m_rayimage = NULL;
    m_rayimagewidth = 0;
    m_rayimageheight = 0;
    m_window = NULL;
}

void CMyRaytraceRenderer::SetImage(BYTE** image, int width, int height)
{
    m_rayimage = image;
    m_rayimagewidth = width;
    m_rayimageheight = height;
}

void CMyRaytraceRenderer::SetWindow(CWnd* p_window)
{
    m_window = p_window;
}