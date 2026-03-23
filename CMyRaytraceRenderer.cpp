#include "pch.h"
#include <cmath>
#include "CMyRaytraceRenderer.h"
#include "graphics/GrTexture.h"

static double ClampUnit(double v)
{
    if (v < 0.0)
    {
        return 0.0;
    }
    if (v > 1.0)
    {
        return 1.0;
    }
    return v;
}

static BYTE ColorToByte(double v)
{
    return BYTE(ClampUnit(v) * 255.0 + 0.5);
}

static CGrPoint Hadamard3(const CGrPoint& a, const CGrPoint& b)
{
    return CGrPoint(a.X() * b.X(), a.Y() * b.Y(), a.Z() * b.Z(), 0);
}

static CGrPoint ReflectDir(const CGrPoint& d, const CGrPoint& n)
{
    return Normalize3(d - n * (2.0 * Dot3(d, n)));
}

static CGrPoint FogColor()
{
    return CGrPoint(0.62, 0.68, 0.78, 0);
}

CMyRaytraceRenderer::CMyRaytraceRenderer()
{
    m_rayimage = NULL;
    m_rayimagewidth = 0;
    m_rayimageheight = 0;
    m_window = NULL;
    m_material = NULL;
    m_maxdepth = 4;
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

    // Keep lights in eye coordinates to match ray space.
    m_lights.clear();
    for (int i = 0; i < LightCnt(); i++)
    {
        CGrRenderer::Light light = GetLight(i);
        if (light.m_pos.W() == 0)
        {
            CGrPoint dir = m_mstack.back() * CGrPoint(light.m_pos.X(), light.m_pos.Y(), light.m_pos.Z(), 0);
            dir = Normalize3(dir);
            light.m_pos.Set(dir.X(), dir.Y(), dir.Z(), 0);
        }
        else
        {
            light.m_pos = m_mstack.back() * light.m_pos;
        }
        m_lights.push_back(light);
    }

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
            // 2x2 supersampling antialiasing
            static const double offsets[2] = { 0.25, 0.75 };
            CGrPoint color(0, 0, 0, 0);
            for (int sy = 0; sy < 2; sy++)
            {
                for (int sx = 0; sx < 2; sx++)
                {
                    const double x = xmin + (double(c) + offsets[sx]) / double(m_rayimagewidth) * xwid;
                    const double y = ymin + (double(r) + offsets[sy]) / double(m_rayimageheight) * ywid;

                    CRay ray(CGrPoint(0, 0, 0), Normalize3(CGrPoint(x, y, -1, 0)));
                    color += TraceRay(ray, NULL, 0);
                }
            }
            color = color * 0.25;

            m_rayimage[r][c * 3 + 0] = ColorToByte(color.X());
            m_rayimage[r][c * 3 + 1] = ColorToByte(color.Y());
            m_rayimage[r][c * 3 + 2] = ColorToByte(color.Z());
        }

        if (m_window != NULL && (r % 20) == 0)
        {
            m_window->Invalidate();
            m_window->UpdateWindow();
        }
    }

    return true;
}

CGrPoint CMyRaytraceRenderer::TraceRay(const CRay& ray, const CRayIntersection::Object* ignore, int depth)
{
    if (depth > m_maxdepth)
    {
        return CGrPoint(0, 0, 0, 0);
    }

    double t = 0;
    CGrPoint point;
    const CRayIntersection::Object* nearest = NULL;
    if (!m_intersection.Intersect(ray, 1e20, ignore, nearest, t, point))
    {
        return CGrPoint(0, 0, 0, 0);
    }

    CGrPoint n;
    CGrMaterial* material = NULL;
    CGrTexture* texture = NULL;
    CGrPoint texcoord;
    m_intersection.IntersectInfo(ray, nearest, t, n, material, texture, texcoord);
    n = Normalize3(n);
    if (Dot3(n, ray.Direction()) > 0)
    {
        n = n * -1.0;
    }

    return Shade(ray, nearest, point, n, material, texture, texcoord, depth);
}

CGrPoint CMyRaytraceRenderer::Shade(const CRay& ray,
    const CRayIntersection::Object* nearest,
    const CGrPoint& point,
    const CGrPoint& normal,
    CGrMaterial* material,
    CGrTexture* texture,
    const CGrPoint& texcoord,
    int depth)
{
    if (material == NULL)
    {
        return CGrPoint(1, 1, 1, 0);
    }

    CGrPoint texColor = TextureColor(texture, texcoord);
    CGrPoint diffuseColor(material->Diffuse(0), material->Diffuse(1), material->Diffuse(2), 0);
    CGrPoint ambientColor(material->Ambient(0), material->Ambient(1), material->Ambient(2), 0);
    diffuseColor = Hadamard3(diffuseColor, texColor);
    ambientColor = Hadamard3(ambientColor, texColor);

    CGrPoint color(0, 0, 0, 0);
    CGrPoint view = Normalize3(ray.Direction() * -1.0);

    // Ambient term from all light ambient contributions.
    for (size_t i = 0; i < m_lights.size(); i++)
    {
        const CGrRenderer::Light& light = m_lights[i];
        color[0] += ambientColor.X() * light.m_ambient[0];
        color[1] += ambientColor.Y() * light.m_ambient[1];
        color[2] += ambientColor.Z() * light.m_ambient[2];
    }

    // Diffuse + specular terms.
    const double epsilon = 1e-4;
    for (size_t i = 0; i < m_lights.size(); i++)
    {
        const CGrRenderer::Light& light = m_lights[i];

        CGrPoint ldir;
        double maxdist = 1e20;
        if (light.m_pos.W() == 0)
        {
            ldir = Normalize3(CGrPoint(light.m_pos.X(), light.m_pos.Y(), light.m_pos.Z(), 0));
        }
        else
        {
            CGrPoint tolight = light.m_pos - point;
            maxdist = tolight.Length3();
            if (maxdist <= 0)
            {
                continue;
            }
            ldir = tolight * (1.0 / maxdist);
        }

        CRay shadowray(point + normal * epsilon, ldir);
        double tshadow = 0;
        CGrPoint pshadow;
        const CRayIntersection::Object* oshadow = NULL;
        const double shadowmaxt = light.m_pos.W() == 0 ? 1e20 : (maxdist - epsilon);
        if (m_intersection.Intersect(shadowray, shadowmaxt, nearest, oshadow, tshadow, pshadow))
        {
            continue;
        }

        const double ndotl = Dot3(normal, ldir);
        if (ndotl <= 0)
        {
            continue;
        }

        color[0] += diffuseColor.X() * light.m_diffuse[0] * ndotl;
        color[1] += diffuseColor.Y() * light.m_diffuse[1] * ndotl;
        color[2] += diffuseColor.Z() * light.m_diffuse[2] * ndotl;

        CGrPoint halfv = Normalize3(ldir + view);
        const double ndoth = Dot3(normal, halfv);
        if (ndoth > 0)
        {
            const double shininess = material->Shininess() < 1.0f ? 1.0 : material->Shininess();
            const double specpow = std::pow(ndoth, shininess);
            color[0] += material->Specular(0) * light.m_specular[0] * specpow;
            color[1] += material->Specular(1) * light.m_specular[1] * specpow;
            color[2] += material->Specular(2) * light.m_specular[2] * specpow;
        }
    }

    // Recursive specular reflection.
    const double reflectivity = ClampUnit((material->SpecularOther(0) +
        material->SpecularOther(1) + material->SpecularOther(2)) / 3.0);
    if (reflectivity > 0.0 && depth < m_maxdepth)
    {
        CGrPoint rdir = ReflectDir(ray.Direction(), normal);
        CRay rray(point + rdir * epsilon, rdir);
        CGrPoint rcolor = TraceRay(rray, nearest, depth + 1);
        color = color * (1.0 - reflectivity) + rcolor * reflectivity;
    }

    // Depth fog / smoke effect.
    const double fogstart = 45.0;
    const double fogend = 140.0;
    const double dist = point.Length3();
    const double fog = ClampUnit((dist - fogstart) / (fogend - fogstart));
    color = color * (1.0 - fog) + FogColor() * fog;

    return CGrPoint(ClampUnit(color.X()), ClampUnit(color.Y()), ClampUnit(color.Z()), 0);
}

CGrPoint CMyRaytraceRenderer::TextureColor(CGrTexture* texture, const CGrPoint& texcoord) const
{
    if (texture == NULL || texture->Empty())
    {
        return CGrPoint(1, 1, 1, 0);
    }

    double s = texcoord.X() - std::floor(texcoord.X());
    double t = texcoord.Y() - std::floor(texcoord.Y());
    if (s < 0.0) s += 1.0;
    if (t < 0.0) t += 1.0;

    int x = int(s * texture->Width());
    int y = int(t * texture->Height());
    if (x >= texture->Width()) x = texture->Width() - 1;
    if (y >= texture->Height()) y = texture->Height() - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    const BYTE* row = texture->Row(y);
    return CGrPoint(row[x * 3 + 0] / 255.0, row[x * 3 + 1] / 255.0, row[x * 3 + 2] / 255.0, 0);
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
    if (m_mstack.empty())
    {
        CGrTransform t;
        t.SetIdentity();
        m_mstack.push_back(t);
    }
    m_mstack.push_back(m_mstack.back());
}

void CMyRaytraceRenderer::RendererPopMatrix()
{
    if (m_mstack.size() > 1)
    {
        m_mstack.pop_back();
    }
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

void CMyRaytraceRenderer::RendererTransform(const CGrTransform* p_transform)
{
    if (p_transform != NULL)
    {
        m_mstack.back() *= *p_transform;
    }
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
