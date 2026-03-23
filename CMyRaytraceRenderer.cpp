#include "pch.h"
#include <cmath>
#include "CMyRaytraceRenderer.h"

static BYTE ClampToByte(double v)
{
    if (v < 0.0)
    {
        return 0;
    }
    if (v > 255.0)
    {
        return 255;
    }
    return BYTE(v);
}

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

bool CMyRaytraceRenderer::RendererEnd()
{
    m_intersection.LoadingComplete();

    if (m_rayimage == NULL || m_rayimagewidth <= 0 || m_rayimageheight <= 0)
    {
        return true;
    }

    const double yhit = std::tan(ProjectionAngle() * GR_DTOR * 0.5);
    const double ymin = -yhit;
    const double ywid = yhit * 2.0;
    const double xwid = ywid * ProjectionAspect();
    const double xmin = -xwid * 0.5;

    for (int r = 0; r < m_rayimageheight; r++)
    {
        for (int c = 0; c < m_rayimagewidth; c++)
        {
            const double x = xmin + (double(c) + 0.5) / double(m_rayimagewidth) * xwid;
            const double y = ymin + (double(r) + 0.5) / double(m_rayimageheight) * ywid;

            // Construct a Ray
            CRay ray(CGrPoint(0, 0, 0), Normalize3(CGrPoint(x, y, -1, 0)));

            double t;                                   // Will be distance to intersection
            CGrPoint intersect;                         // Will by x,y,z location of intersection
            const CRayIntersection::Object* nearest;    // Pointer to intersecting object
            if (m_intersection.Intersect(ray, 1e20, NULL, nearest, t, intersect))
            {
                // We hit something...
                // Determine information about the intersection
                CGrPoint N;
                CGrMaterial *material;
                CGrTexture *texture;
                CGrPoint texcoord;

                m_intersection.IntersectInfo(ray, nearest, t,
                                             N, material, texture, texcoord);

                if (material != NULL)
                {
                    m_rayimage[r][c * 3 + 0] = ClampToByte(material->Diffuse(0) * 255.0);
                    m_rayimage[r][c * 3 + 1] = ClampToByte(material->Diffuse(1) * 255.0);
                    m_rayimage[r][c * 3 + 2] = ClampToByte(material->Diffuse(2) * 255.0);
                }
                else
                {
                    m_rayimage[r][c * 3 + 0] = BYTE(255);
                    m_rayimage[r][c * 3 + 1] = BYTE(255);
                    m_rayimage[r][c * 3 + 2] = BYTE(255);
                }
            }
            else
            {
                // We hit nothing...
                m_rayimage[r][c * 3 + 0] = 0;
                m_rayimage[r][c * 3 + 1] = 0;
                m_rayimage[r][c * 3 + 2] = 0;
            }
        }
    }

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
