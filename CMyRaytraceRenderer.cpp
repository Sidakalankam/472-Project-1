#include "pch.h"
#include "CMyRaytraceRenderer.h"

CMyRaytraceRenderer::CMyRaytraceRenderer()
{
    m_rayimage = NULL;
    m_rayimagewidth = 0;
    m_rayimageheight = 0;
    m_window = NULL;
    m_material = NULL;
}

bool CMyRaytraceRenderer::RendererStart()
{
    m_intersection.Initialize();
    m_mstack.clear();

    // We have to do all of the matrix work ourselves.
    // Set up the matrix stack.
    CGrTransform t;
    t.SetLookAt(Eye().X(), Eye().Y(), Eye().Z(),
                Center().X(), Center().Y(), Center().Z(),
                Up().X(), Up().Y(), Up().Z());

    m_mstack.push_back(t);
    m_material = NULL;

    return true;
}

//
// Name : CMyRaytraceRenderer::RendererEndPolygon()
// Description : End definition of a polygon. The superclass has
// already collected the polygon information
//
void CMyRaytraceRenderer::RendererEndPolygon()
{
    const std::list<CGrPoint>& vertices = PolyVertices();
    const std::list<CGrPoint>& normals = PolyNormals();
    const std::list<CGrPoint>& tvertices = PolyTexVertices();

    // Allocate a new polygon in the ray intersection system
    m_intersection.PolygonBegin();
    m_intersection.Material(m_material);

    if (PolyTexture())
    {
        m_intersection.Texture(PolyTexture());
    }

    std::list<CGrPoint>::const_iterator normal = normals.begin();
    std::list<CGrPoint>::const_iterator tvertex = tvertices.begin();

    for (std::list<CGrPoint>::const_iterator i = vertices.begin(); i != vertices.end(); i++)
    {
        if (normal != normals.end())
        {
            m_intersection.Normal(m_mstack.back() * *normal);
            normal++;
        }

        if (tvertex != tvertices.end())
        {
            m_intersection.TexVertex(*tvertex);
            tvertex++;
        }

        m_intersection.Vertex(m_mstack.back() * *i);
    }

    m_intersection.PolygonEnd();
}

void CMyRaytraceRenderer::RendererMaterial(CGrMaterial* p_material)
{
    m_material = p_material;
}

void CMyRaytraceRenderer::RendererPushMatrix()
{
    m_mstack.push_back(m_mstack.back());
}

void CMyRaytraceRenderer::RendererPopMatrix()
{
    m_mstack.pop_back();
}

void CMyRaytraceRenderer::RendererRotate(double angle, double x, double y, double z)
{
    CGrTransform r;
    r.SetRotate(angle, CGrPoint(x, y, z));
    m_mstack.back() *= r;
}

void CMyRaytraceRenderer::RendererTranslate(double x, double y, double z)
{
    CGrTransform t;
    t.SetTranslate(x, y, z);
    m_mstack.back() *= t;
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
