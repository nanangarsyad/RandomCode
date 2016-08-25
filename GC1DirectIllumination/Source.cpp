#define _CRT_SECURE_NO_WARNINGS
 
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <array>
#include <thread>
#include <atomic>

#include "SVector.h"
#include "SSphere.h"
#include "STriangle.h"
#include "SMaterial.h"
#include "SImageData.h"
#include "Utils.h"
#include "STimer.h"
#include "Random.h"

//=================================================================================
//                                 SETTINGS
//=================================================================================

// image size
static const size_t c_imageWidth = 1024;
static const size_t c_imageHeight = 1024;

// sampling
static const size_t c_samplesPerPixel = 100;
static const bool c_jitterSamples = true && (c_samplesPerPixel > 1);
static const size_t c_maxBounces = 8;
static const size_t c_russianRouletteStartBounce = 4;

// threading toggle
static const bool c_forceSingleThreaded = false;

// camera - assumes no roll, and that (0,1,0) is up
static const SVector c_cameraPos = { -3.0f, 2.0f, -10.0f };
static const SVector c_cameraAt = { 0.0f, 0.0f, 0.0f };
static const float c_nearDist = 0.1f;
static const float c_cameraVerticalFOV = 60.0f * c_pi / 180.0f;

// Materials - name, diffuse, emissive, reflective, refractive, refractionIndex, brdf
#define MATERIALLIST() \
    MATERIAL(MatteRed      , SVector(0.9f, 0.1f, 0.1f), SVector(), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(MatteGreen    , SVector(0.1f, 0.9f, 0.1f), SVector(), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(MatteBlue     , SVector(0.1f, 0.1f, 0.9f), SVector(), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(MatteTeal     , SVector(0.1f, 0.9f, 0.9f), SVector(), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(MatteMagenta  , SVector(0.9f, 0.1f, 0.9f), SVector(), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(MatteYellow   , SVector(0.9f, 0.9f, 0.1f), SVector(), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(Chrome        , SVector(0.01f, 0.01f, 0.01f), SVector(), SVector(1.0f, 1.0f, 1.0f), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(EmissiveRed   , SVector(), SVector(0.9f, 0.3f, 0.3f), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(EmissiveGreen , SVector(), SVector(0.3f, 0.9f, 0.3f), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(EmissiveBlue  , SVector(), SVector(0.3f, 0.3f, 0.9f), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(EmissiveWhite , SVector(), SVector(1.0f, 1.0f, 1.0f), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(Walls         , SVector(0.5f, 0.5f, 0.5f), SVector(), SVector(), SVector(), 1.0f, EBRDF::standard) \
    MATERIAL(Water         , SVector(), SVector(), SVector(), SVector(1.0f, 1.0f, 1.0f), 1.1f /*1.3f*/, EBRDF::standard) \

#include "MakeMaterials.h"

// Spheres
auto c_spheres = make_array(
    SSphere(SVector(-2.0f, 0.0f, 4.0f), 2.0f, TMaterialID::Chrome),
    SSphere(SVector(-1.5f, 1.0f, -2.0f), 1.0f, TMaterialID::Water),
    SSphere(SVector(0.0f, 0.0f, 2.0f), 0.5f, TMaterialID::MatteRed),
    SSphere(SVector(-2.0f, 0.0f, 2.0f), 0.5f, TMaterialID::MatteGreen),
    SSphere(SVector(2.0f, 0.0f, 2.0f), 0.5f, TMaterialID::MatteBlue),
    SSphere(SVector(0.0f, 2.0f, 2.0f), 0.5f, TMaterialID::MatteTeal),
    SSphere(SVector(0.0f,-2.0f, 2.0f), 0.5f, TMaterialID::MatteMagenta),

    SSphere(SVector(0.5f, 0.1f, 0.0f), 0.03f, TMaterialID::EmissiveWhite),     // red light
    SSphere(SVector(0.3f, -0.3f, 0.0f), 0.03f, TMaterialID::EmissiveWhite),   // green light
    SSphere(SVector(-0.3f, 0.1f, -1.0f), 0.03f, TMaterialID::EmissiveWhite)   // blue light
);

const float c_boxSize = 5.0f;

// Triangles
auto c_triangles = make_array(
    STriangle(SVector(1.5f, 1.0f, 1.25f), SVector(1.5f, 0.0f, 1.75f), SVector(0.5f, 0.0f, 2.25f), TMaterialID::MatteYellow),
    STriangle(SVector(0.5f, 0.25f, 3.5f), SVector(1.5f, 0.25f, 3.0f), SVector(1.5f, 1.25f, 2.5f), TMaterialID::MatteYellow),

    // box wall - in front
    STriangle(SVector(-c_boxSize, -c_boxSize,  c_boxSize), SVector( c_boxSize, -c_boxSize,  c_boxSize), SVector( c_boxSize,  c_boxSize,  c_boxSize), TMaterialID::Walls),
    STriangle(SVector(-c_boxSize, -c_boxSize,  c_boxSize), SVector(-c_boxSize,  c_boxSize,  c_boxSize), SVector( c_boxSize,  c_boxSize,  c_boxSize), TMaterialID::Walls),

    // box wall - left
    STriangle(SVector(-c_boxSize, -c_boxSize,  c_boxSize), SVector(-c_boxSize, -c_boxSize, -c_boxSize), SVector(-c_boxSize, c_boxSize, -c_boxSize), TMaterialID::Walls),
    STriangle(SVector(-c_boxSize, -c_boxSize,  c_boxSize), SVector(-c_boxSize,  c_boxSize,  c_boxSize), SVector(-c_boxSize, c_boxSize, -c_boxSize), TMaterialID::Walls),

    // box wall - right
    STriangle(SVector( c_boxSize, -c_boxSize,  c_boxSize), SVector( c_boxSize, -c_boxSize, -c_boxSize), SVector( c_boxSize, c_boxSize, -c_boxSize), TMaterialID::Walls),
    STriangle(SVector( c_boxSize, -c_boxSize,  c_boxSize), SVector( c_boxSize,  c_boxSize,  c_boxSize), SVector( c_boxSize, c_boxSize, -c_boxSize), TMaterialID::Walls),

    // box wall - bottom
    STriangle(SVector(-c_boxSize, -c_boxSize,  c_boxSize), SVector(-c_boxSize, -c_boxSize, -c_boxSize), SVector( c_boxSize, -c_boxSize, -c_boxSize), TMaterialID::Walls),
    STriangle(SVector(-c_boxSize, -c_boxSize,  c_boxSize), SVector( c_boxSize, -c_boxSize,  c_boxSize), SVector( c_boxSize, -c_boxSize, -c_boxSize), TMaterialID::Walls),

    // box wall - top
    STriangle(SVector(-c_boxSize,  c_boxSize,  c_boxSize), SVector(-c_boxSize,  c_boxSize, -c_boxSize), SVector( c_boxSize,  c_boxSize, -c_boxSize), TMaterialID::Walls),
    STriangle(SVector(-c_boxSize,  c_boxSize,  c_boxSize), SVector( c_boxSize,  c_boxSize,  c_boxSize), SVector( c_boxSize,  c_boxSize, -c_boxSize), TMaterialID::Walls)

    // box wall - behind
    //STriangle(SVector(-c_boxSize, -c_boxSize, -c_boxSize), SVector( c_boxSize, -c_boxSize, -c_boxSize), SVector( c_boxSize,  c_boxSize, -c_boxSize), TMaterialID::Walls),
    //STriangle(SVector(-c_boxSize, -c_boxSize, -c_boxSize), SVector(-c_boxSize,  c_boxSize, -c_boxSize), SVector( c_boxSize,  c_boxSize, -c_boxSize), TMaterialID::Walls)
);

//=================================================================================
static SVector CameraRight ()
{
    SVector cameraFwd = c_cameraAt - c_cameraPos;
    Normalize(cameraFwd);

    SVector cameraRight = Cross(SVector(0.0f, 1.0f, 0.0f), cameraFwd);
    Normalize(cameraRight);

    SVector cameraUp = Cross(cameraFwd, cameraRight);
    Normalize(cameraUp);

    return cameraRight;
}

//=================================================================================
static SVector CameraUp ()
{
    SVector cameraFwd = c_cameraAt - c_cameraPos;
    Normalize(cameraFwd);

    SVector cameraRight = Cross(SVector(0.0f, 1.0f, 0.0f), cameraFwd);
    Normalize(cameraRight);

    SVector cameraUp = Cross(cameraFwd, cameraRight);
    Normalize(cameraUp);

    return cameraUp;
}

//=================================================================================
static SVector CameraFwd ()
{
    SVector cameraFwd = c_cameraAt - c_cameraPos;
    Normalize(cameraFwd);

    SVector cameraRight = Cross(SVector(0.0f, 1.0f, 0.0f), cameraFwd);
    Normalize(cameraRight);

    SVector cameraUp = Cross(cameraFwd, cameraRight);
    Normalize(cameraUp);

    return cameraFwd;
}

//=================================================================================
static const size_t c_numPixels = c_imageWidth * c_imageHeight;
static const float c_aspectRatio = float(c_imageWidth) / float(c_imageHeight);
static const float c_cameraHorizFOV = c_cameraVerticalFOV * c_aspectRatio;
static const float c_windowTop = tan(c_cameraVerticalFOV / 2.0f) * c_nearDist;
static const float c_windowRight = tan(c_cameraHorizFOV / 2.0f) * c_nearDist;
static const SVector c_cameraRight = CameraRight();
static const SVector c_cameraUp = CameraUp();
static const SVector c_cameraFwd = CameraFwd();
static const float c_brightnessAdjust = 1.0f / (float)c_samplesPerPixel;
static const size_t c_RRBounceLeftBegin = c_maxBounces > c_russianRouletteStartBounce ? c_maxBounces - c_russianRouletteStartBounce : 0;

std::atomic<bool> g_wantsExit(false);

// make an RGB f32 texture.
// lower left of image is (0,0).
static SImageDataRGBF32 g_image_RGB_F32(c_imageWidth, c_imageHeight);

//=================================================================================
bool AnyIntersection (const SVector& a, const SVector& dir, float length, TObjectID ignoreObjectID = c_invalidObjectID)
{
    SCollisionInfo collisionInfo;
    collisionInfo.m_maxCollisionTime = length;
    for (const SSphere& s : c_spheres)
    {
        if (RayIntersects(a, dir, s, collisionInfo, ignoreObjectID))
            return true;
    }
    for (const STriangle& t : c_triangles)
    {
        if (RayIntersects(a, dir, t, collisionInfo, ignoreObjectID))
            return true;
    }

    return false;
}

//=================================================================================
bool ClosestIntersection (const SVector& rayPos, const SVector& rayDir, SCollisionInfo& collisionInfo, TObjectID ignoreObjectID = c_invalidObjectID)
{
    bool ret = false;
    for (const SSphere& s : c_spheres)
        ret |= RayIntersects(rayPos, rayDir, s, collisionInfo, ignoreObjectID);
    for (const STriangle& t : c_triangles)
        ret |= RayIntersects(rayPos, rayDir, t, collisionInfo, ignoreObjectID);
    return ret;
}

//=================================================================================
inline bool PassesRussianRoulette (const SVector& v, size_t bouncesLeft)
{
    // if the vector is 0, it fails Russian roulette test always
    if (!NotZero(v))
        return false;

    // otherwise, only test if we are past the point of when Russian roulette should start
    if (bouncesLeft >= c_RRBounceLeftBegin)
        return true;

    // else leave it to chance based on the magnitude of the largest component of the vector
    return RandomFloat() <= MaxComponentValue(v);
}

//=================================================================================
SVector L_out(const SCollisionInfo& X, const SVector& outDir, size_t bouncesLeft)
{
    // if no bounces left, return black / darkness
    if (bouncesLeft == 0)
        return SVector();

    const SMaterial& material = c_materials[(size_t)X.m_materialID];

    // start with emissive lighting
    SVector ret = material.m_emissive;

    // Add in BRDF pulses.
    // Pulses are strong sampling points which on the BRDF which would be basically impossible to hit with monte carlo.

    // add in reflection BRDF pulse.  
    if (PassesRussianRoulette(material.m_reflection, bouncesLeft))
    {
        SVector reflectDir = Reflect(-outDir, X.m_surfaceNormal);
        SCollisionInfo collisionInfo;
        if (ClosestIntersection(X.m_intersectionPoint, reflectDir, collisionInfo, X.m_objectID))
        {
            float cos_theta = Dot(reflectDir, X.m_surfaceNormal);
            SVector BRDF = 2.0f * material.m_reflection * cos_theta;
            ret += BRDF * L_out(collisionInfo, -reflectDir, bouncesLeft - 1);
        }
    }

    // add in refraction BRDF pulse.
    if (PassesRussianRoulette(material.m_refraction, bouncesLeft))
    {
        // make our refraction index ratio.
        // air has a refractive index of just over 1.0, and vacum has 1.0.
        float ratio = X.m_fromInside ? material.m_refractionIndex / 1.0f : 1.0f / material.m_refractionIndex;
        SVector refractDir = Refract(-outDir, X.m_surfaceNormal, ratio);
        SCollisionInfo collisionInfo;

        // We need to push the ray out a little bit, instead of telling it to ignore this object for the intersection
        // test, because we may hit the same object again legitimately!
        if (ClosestIntersection(X.m_intersectionPoint + refractDir * 0.001f, refractDir, collisionInfo))
        {
            float cos_theta = Dot(refractDir, X.m_surfaceNormal);
            SVector BRDF = 2.0f * material.m_refraction * cos_theta;
            ret += BRDF * L_out(collisionInfo, -refractDir, bouncesLeft - 1);
        }
    }

    // add in random samples for global illumination etc
    if (PassesRussianRoulette(material.m_diffuse, bouncesLeft))
    {
        SVector newRayDir = RandomUnitVectorInHemisphere(X.m_surfaceNormal);
        SCollisionInfo collisionInfo;
        if (ClosestIntersection(X.m_intersectionPoint, newRayDir, collisionInfo, X.m_objectID))
        {
            float cos_theta = Dot(newRayDir, X.m_surfaceNormal);
            SVector BRDF = 2.0f * material.m_diffuse * cos_theta;
            ret += BRDF * L_out(collisionInfo, -newRayDir, bouncesLeft - 1);
        }
    }
    return ret;
}

//=================================================================================
SVector L_in (const SVector& X, const SVector& dir)
{
    // if this ray doesn't hit anything, return black / darkness
    SCollisionInfo collisionInfo;
    if (!ClosestIntersection(X, dir, collisionInfo))
        return SVector();

    // else, return the amount of light coming towards us from the object we hit
    return L_out(collisionInfo, -dir, c_maxBounces);
}

//=================================================================================
void RenderPixel (float u, float v, SVector& pixel)
{
    // make (u,v) go from [-1,1] instead of [0,1]
    u = u * 2.0f - 1.0f;
    v = v * 2.0f - 1.0f;

    // find where the ray hits the near plane, and normalize that vector to get the ray direction.
    SVector rayStart = c_cameraPos + c_cameraFwd * c_nearDist;
    rayStart += c_cameraRight * c_windowRight * u;
    rayStart += c_cameraUp * c_windowTop * v;
    SVector rayDir = rayStart - c_cameraPos;
    Normalize(rayDir);

    // our pixel is the amount of light coming in to the position on our near plane from the ray direction.
    pixel = L_in(rayStart, rayDir);
}

//=================================================================================
void ThreadFunc (STimer& timer)
{
    static std::atomic<size_t> g_currentPixelIndex(-1);

    // render individual pixels across multiple threads until we run out of pixels to do
    size_t pixelIndex = ++g_currentPixelIndex;
    bool firstThread = pixelIndex == 0;
    while (pixelIndex < g_image_RGB_F32.m_pixels.size() && !g_wantsExit.load())
    {
        // get the current pixel's UV and actual memory location
        size_t x = pixelIndex % g_image_RGB_F32.m_width;
        size_t y = pixelIndex / g_image_RGB_F32.m_height;
        float xPercent = (float)x / (float)g_image_RGB_F32.m_width;
        float yPercent = (float)y / (float)g_image_RGB_F32.m_height;
        RGB_F32& pixel = g_image_RGB_F32.m_pixels[pixelIndex];

        // render the current pixel by averaging (possibly) multiple (possibly) jittered samples.
        // jitter by +/- half a pixel.
        for (size_t i = 0; i < c_samplesPerPixel; ++i)
        {
            float jitterX = 0.0f;
            float jitterY = 0.0f;
            if (c_jitterSamples)
            {
                jitterX = (RandomFloat() - 0.5f) / (float)g_image_RGB_F32.m_width;
                jitterY = (RandomFloat() - 0.5f) / (float)g_image_RGB_F32.m_height;
            }

            SVector out;
            RenderPixel(xPercent + jitterX, yPercent + jitterY, out);

            pixel[0] += out.m_x * c_brightnessAdjust;
            pixel[1] += out.m_y * c_brightnessAdjust;
            pixel[2] += out.m_z * c_brightnessAdjust;
        }

        // apply SRGB correction
        pixel[0] = powf(pixel[0], 1.0f / 2.2f);
        pixel[1] = powf(pixel[1], 1.0f / 2.2f);
        pixel[2] = powf(pixel[2], 1.0f / 2.2f);

        // move to next pixel
        pixelIndex = ++g_currentPixelIndex;

        // first thread reports progress, show what percent we are at
        if (firstThread)
            timer.ReportProgress(pixelIndex, g_image_RGB_F32.m_pixels.size());
    }
}


//=================================================================================
void CaptureImage (SImageDataBGRU8& dest, const SImageDataRGBF32& src)
{
    for (size_t y = 0; y < c_imageHeight; ++y)
    {
        const RGB_F32 *srcPixel = &src.m_pixels[y*src.m_width];
        BGR_U8 *destPixel = (BGR_U8*)&dest.m_pixels[y*dest.m_pitch];
        for (size_t x = 0; x < c_imageWidth; ++x)
        {
            (*destPixel)[0] = uint8(Clamp((*srcPixel)[2] * 255.0f, 0.0f, 255.0f));
            (*destPixel)[1] = uint8(Clamp((*srcPixel)[1] * 255.0f, 0.0f, 255.0f));
            (*destPixel)[2] = uint8(Clamp((*srcPixel)[0] * 255.0f, 0.0f, 255.0f));
            ++srcPixel;
            ++destPixel;
        }
    }
}

//=================================================================================
LRESULT __stdcall WindowProcedure (HWND window, unsigned int msg, WPARAM wp, LPARAM lp)
{
    static HBITMAP hbitmap = (HBITMAP)LoadImageA(nullptr, "out_3000.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    static SImageDataBGRU8 image_BGR_U8(c_imageWidth, c_imageHeight);
    switch (msg)
    {
        case WM_TIMER:
        {
            /*
            DeleteObject(hbitmap);

            // capture image
            
            CaptureImage(image_BGR_U8, g_image_RGB_F32);

            // remake bitmap
            BITMAPINFOHEADER bmih;
            bmih.biSize = sizeof(BITMAPINFOHEADER);
            bmih.biWidth = c_imageWidth;
            bmih.biHeight = c_imageHeight;
            bmih.biPlanes = 1;
            bmih.biBitCount = 24;
            bmih.biCompression = BI_RGB;
            bmih.biSizeImage = 0;
            bmih.biXPelsPerMeter = 10;
            bmih.biYPelsPerMeter = 10;
            bmih.biClrUsed = 0;
            bmih.biClrImportant = 0;

            BITMAPINFO dbmi;
            ZeroMemory(&dbmi, sizeof(dbmi));
            dbmi.bmiHeader = bmih;
            dbmi.bmiColors->rgbBlue = 0;
            dbmi.bmiColors->rgbGreen = 0;
            dbmi.bmiColors->rgbRed = 0;
            dbmi.bmiColors->rgbReserved = 0;

            HDC temphdc = ::GetDC(NULL);

            HBITMAP hbmp = CreateDIBitmap(temphdc, &bmih, CBM_INIT, &image_BGR_U8.m_pixels[0], &dbmi, DIB_RGB_COLORS);
            ::ReleaseDC(NULL, temphdc);
            */

            RedrawWindow(window, nullptr, nullptr, RDW_INTERNALPAINT);
            return DefWindowProc(window, msg, wp, lp);
        }
        case WM_PAINT:
        {
            /*
            SImageDataBGRU8 image_BGR_U8(c_imageWidth, c_imageHeight);
            CaptureImage(image_BGR_U8, g_image_RGB_F32);
            //SaveImage("outthread.bmp", image_BGR_U8);
            // TODO: make a bitmap and render it to the window
            // This looks promising: http://www.cplusplus.com/forum/windows/27187/
            // also this: http://stackoverflow.com/questions/1748470/how-to-draw-image-on-a-window

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(window, &ps);

            //HDC hdc = GetDC(window); // TODO: release?

            BITMAPINFO info;
            info.bmiHeader.biSize = sizeof(info.bmiHeader);
            info.bmiHeader.biWidth = c_imageWidth;
            info.bmiHeader.biHeight = c_imageHeight;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 24; // 24 bits per pixel - one unsigned char for each pixel
            info.bmiHeader.biCompression = BI_RGB;
            info.bmiHeader.biSizeImage = 0;
            info.bmiHeader.biClrUsed = 0;
            info.bmiHeader.biClrImportant = 0;
            HDC cDC = CreateCompatibleDC(hdc); // this is the GetDC (hwnd) where hwnd is the
            // handle of the window I want to write to
            HBITMAP hbmp = CreateCompatibleBitmap(hdc, c_imageWidth, c_imageHeight);
            SetDIBits(hdc, hbmp, 0, c_imageHeight, &image_BGR_U8.m_pixels[0], &info, DIB_RGB_COLORS);



            hbmp = (HBITMAP)SelectObject(cDC, hbmp);
            BitBlt(hdc, 0, 0, c_imageWidth, c_imageHeight, cDC, 0, 0, SRCCOPY);
            DeleteObject(SelectObject(cDC, hbmp));
            DeleteDC(cDC);

            //ReleaseDC(window, hdc);

            EndPaint(window, &ps);


            DeleteObject(hbmp);
            */

            // TODO: maybe make image_BGR_U8 static so it doesn't keep reallocating
            // TODO: profile window thread and see if it can be improved

            




            // draw bitmap

            PAINTSTRUCT 	ps;
            HDC 			hdc;
            BITMAP 			bitmap;
            HDC 			hdcMem;
            HGDIOBJ 		oldBitmap;

            hdc = BeginPaint(window, &ps);

            hdcMem = CreateCompatibleDC(hdc);
            oldBitmap = SelectObject(hdcMem, hbitmap);

            GetObject(hbitmap, sizeof(bitmap), &bitmap);
            BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, oldBitmap);
            DeleteDC(hdcMem);

            EndPaint(window, &ps);

            return 0;
        }
        case WM_CLOSE:
        {
            g_wantsExit = true;
            return 0;
        }
        default:
        {
            return DefWindowProc(window, msg, wp, lp);
        }
    }
}

//=================================================================================
void WindowFunc ()
{
    WNDCLASSEX wndclass = { sizeof(WNDCLASSEX), CS_DBLCLKS, WindowProcedure,
                            0, 0, GetModuleHandle(0), LoadIcon(0,IDI_APPLICATION),
                            LoadCursor(0,IDC_ARROW), HBRUSH(COLOR_WINDOW+1),
                            0, L"myclass", LoadIcon(0,IDI_APPLICATION) } ;
    if( RegisterClassEx(&wndclass) )
    {
        HWND window = CreateWindowEx( 0, L"myclass", L"title",
            (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU), CW_USEDEFAULT, CW_USEDEFAULT,
                    c_imageWidth, c_imageHeight, 0, 0, GetModuleHandle(0), 0 ) ;
        if(window)
        {
            // start a timer, for us to get the latest rendered image
            SetTimer(window, 0, 1000, nullptr);

            ShowWindow( window, SW_SHOWDEFAULT ) ;
            MSG msg ;
            while( GetMessage( &msg, 0, 0, 0 ) && !g_wantsExit.load())
                DispatchMessage(&msg) ;
        }
    }
}

//=================================================================================
int main (int argc, char **argv)
{
    // Render to the image
    {
        STimer Timer("Render Time");

        // spin up some threads to do work, and wait for them to be finished.
        size_t numThreads = std::thread::hardware_concurrency() - 1;  // TODO: try not subtracting 1? i wonder what will happen... may make window less responsive?
        if (c_forceSingleThreaded || numThreads < 1)
            numThreads = 1;
        printf("Spinning up %i threads to make a %i x %i image.\n", numThreads, c_imageWidth, c_imageHeight);
        printf("%i samples per pixel, %i max bounces.\n", c_samplesPerPixel, c_maxBounces);
        std::vector<std::thread> threads;
        threads.resize(numThreads);
        for (std::thread& t : threads)
            t = std::thread(ThreadFunc, std::ref(Timer));
        std::thread windowThread(WindowFunc);
        windowThread.join();
        for (std::thread& t : threads)
            t.join();
    }

    // Convert from RGB floating point to BGR u8
    SImageDataBGRU8 image_BGR_U8(c_imageWidth, c_imageHeight);
    CaptureImage(image_BGR_U8, g_image_RGB_F32);

    // save the image
    SaveImage("out.bmp", image_BGR_U8);
    WaitForEnter();
    return 0;
}

/*

NOW:
* make a window that shows the image as it's being made
* don't exit the app til the window is closed
* make a button to save a screenshot
* update the image at some rate
* make the window title show data? (number of samples?)
 * console window probably needs to be reworked too
* maybe have one thread handle the window creation / updating?
 * not sure how single threaded would work in that scenario though.
* have the window thread report progress, instead of the first render thread!
* we somehow need the threads to have the threads keep iterating over the pixels over and over
* move window stuff into it's own file?

NEXT:
* get BRDFs working
 * for now, just choose BDRF type (reflect, refract, diffuse)
 * then combine them after they are working

* related to refraction:
 * Total internal reflection
 * fresnel

* Objects inside of transparent objects are problematic, need to fix.
  * not sure why!

 BRDF stuff:
 * generalize reflect / refract pulses, or leave alone?
 * and whatever else part of the brdf?
 * roughness?

GRAPHICS FEATURES:
* are you handling BRDF pulses correctly? seems like reflect / refract maybe shouldn't be on par with diffuse.
* maybe make a cube type?  could replace the room walls with a cube then even!
* make it so you can do uniform samples instead of random samples, to have a graphical comparison of results.
 * Should make it obvious why monte carlo is the better way
* fresnel: graphics codex talks about fresnel in material section
* smallpt handles glass vs mirrors vs diffuse surfaces differently
 * https://drive.google.com/file/d/0B8g97JkuSSBwUENiWTJXeGtTOHFmSm51UC01YWtCZw/view
* implement roughness somehow
* try mixing direct illumination with monte carlo like this: https://www.shadertoy.com/view/4tl3z4
* better source of random numbers than rand
* scattering function
* importance sampling
* ! multiple importance sampling
* dont forget to tone map to get from whatever floating point values back to 0..1 before going to u8
 * saturate towards white!
* bloom (post process)
* other primitive types?
* CSG
* refraction
* beer's law / internal reflection stuff
* participating media (fog)
* blue noise sampling?
 * try different jittering patterns
 * could even try blue noise dithered thing!
* subsurface scattering
* bokeh
* depth of field
* motion blur (monte carlo sample in time, not just in space)
* load and render meshes
* textures
* bump mapping
? look up "volumetric path tracing"?  https://en.wikipedia.org/wiki/Volumetric_path_tracing
* area lights and image based lighting? this may just work, by having emissive surfaces / textures.
* chromatic abberation etc (may need to do frequency sampling!!)
* adaptive rendering? render at low res, then progressively higher res? look into how that works.
* red / blue 3d glasses mode
? how to address color banding? or is there even banding?
? linearly transformed cosines?
* ggx and spherical harmonics
* ccvt sampling and other stuff from "rolling the dice" siggraph talk
* could look at smallpt line by line for parity
 * https://drive.google.com/file/d/0B8g97JkuSSBwUENiWTJXeGtTOHFmSm51UC01YWtCZw/view
* spatial acceleration structure could be helpful perhaps, especially when more objects added
* get importance sampling working, to get reflection working for scattering / BRDF
* importance sampling: https://inst.eecs.berkeley.edu/~cs294-13/fa09/lectures/scribe-lecture5.pdf
* make it so you can render multiple frames and it puts them together into a video

SCENE:
* add a skybox?

OTHER:
* make filename be based on resolution, samples and bounce count?
* make it print out resolution, samples, bounce count, primitive count in the window as it's processing
* make it print an estimated time remaining of render based on percentage done and how long it took to get there?
* try to make it so you give a thread an entire row to do.  May be faster?
* do TODO's in code files
* visualize # of raybounces, instead of colors, for complexity analysis?
 * maybe defines or settings to do this?
 * also visualize normals and reflection bounces or something?
* make a window that shows the progress as it goes.
 * a button to take a screenshot
 * show # of samples etc in top of window
 * maybe click on a pixel to re-render it for debugging purposes
* make it so you can animate things & camera over time at a specified frame rate.  write each frame to disk. combine with ffmpeg to make videos!
* make width / height be command line options?
* aspect ratio support is weird. it stretches images in a funny way.  may be correct?
* profile with sleepy to see where the time is going!

! blog posts on all this info
 * basic path tracing / rendering equation
 * advanced features like russian roulette and such
 * specific features

*/