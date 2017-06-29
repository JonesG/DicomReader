#include <iostream>
#include <assert.h>
#include <algorithm>

#include "DICOMParser.h"
#include "DICOMAppHelper.h"

#include "tinydir.h"
#include "VTKWriter.h"

void writePPM( std::string & strFileName, const int16_t * pBuffer, const int dimx, const int dimy )
{
    // find max/min
    int32_t iMin = _I32_MAX;
    int32_t iMax = _I32_MIN;
    int     iNum = dimx*dimy;
    for ( int i = 0; i < iNum; i++ )
    {
        const int32_t   iVal = pBuffer[i];
        iMin = min( iMin, iVal );
        iMax = max( iMax, iVal );
    }
    int16_t iScale = iMax - iMin;

    std::cout << "Min = " << iMin << " Max = " << iMax << "\n";

    FILE *fp = fopen(strFileName.c_str(), "wb");

    (void)fprintf(fp, "P6\n%d %d\n255\n", dimx, dimy);

    int32_t   iMinP = INT32_MAX;
    int32_t   iMaxP = INT32_MIN;
    for ( int i = 0; i < iNum; i++ )
    {
       static unsigned char color[3];
       const int32_t   iVal = pBuffer[i];
       unsigned char cVal = static_cast<unsigned char>(((iVal - iMin) * 255) / iScale);
       iMinP = min(iMinP, int(cVal));
       iMaxP = max(iMaxP, int(cVal));
       color[0] = cVal; // red
       color[1] = cVal; // green
       color[2] = cVal; // blue
       (void)fwrite(color, 1, 3, fp);
    }

    (void)fclose(fp);

    std::cout << "MinP = " << iMinP << " MaxP = " << iMaxP << "\n";
}

bool ReadDicomFile( const std::string &     strFilename,
                    DICOMParser &           parser,
                    DICOMAppHelper &        helper,
                    void *                  pvBuffer = NULL )
{
    if ( !parser.OpenFile(dicom_stl::string( strFilename.c_str() )))
    {
        std::cout << "Couldn't open " << strFilename << "\n";
        return false;
    }

    if (!parser.ReadHeader())
    {
        std::cout << "Couldn't read dicom header\nPress any key\n";
        return false;
    }

    DICOMParser::VRTypes    dataType;
    unsigned long           len = 0;

    helper.GetImageData(pvBuffer, dataType, len);

    // 0 == unsigned, 1 == signed
    if (helper.GetPixelRepresentation() == 1)
    {
        int16_t *  pBuffer = reinterpret_cast<int16_t *>(pvBuffer);
        static int i = 0;
        std::string strPPMFilename = std::string("test") + std::to_string(i++) + std::string(".ppm");
        writePPM( strPPMFilename, pBuffer, helper.GetWidth(), helper.GetHeight() );
    }

    return true;
}

bool ReadDicomDir(  const std::string &     strDicomDir,
                    DICOMParser &           parser,
                    DICOMAppHelper &        helper,
                    void *                  pvBuffer    )
{
    tinydir_dir dir;
    if (tinydir_open(&dir, strDicomDir.c_str()) == -1)
    {
        perror("Error opening dicom dir\n");
        return false;
    }

    while (dir.has_next)
    {
        tinydir_file file;
        if (tinydir_readfile(&dir, &file) == -1)
        {
            perror("Error getting file\n");
            tinydir_close(&dir);
            return false;
        }

        //printf("%s\n", file.name);
        if ( !file.is_dir )
        {
            bool    bOk = ReadDicomFile(strDicomDir + '\\' + std::string(file.name), parser, helper, pvBuffer );
        }

        if (tinydir_next(&dir) == -1)
        {
            perror("Error getting next file");
            tinydir_close(&dir);
            return false;
        }
    }

    tinydir_close(&dir);
    return true;
}

bool GetDicomDirDimensions( const std::string &     strDicomDir,
                            DICOMParser &           parser,
                            DICOMAppHelper &        helper,
                            void *                  pvBuffer,
                            uint32_t &              uiNumSlices,
                            float &                 fXSpacing,
                            float &                 fYSpacing,
                            float &                 fZSpacing    )
{
    tinydir_dir dir;
    if (tinydir_open(&dir, strDicomDir.c_str()) == -1)
    {
        perror("Error opening dicom dir\n");
        return false;
    }

    uiNumSlices = 0;

    std::vector<float>  vfZ;

    while (dir.has_next)
    {
        tinydir_file file;
        if (tinydir_readfile(&dir, &file) == -1)
        {
            perror("Error getting file\n");
            tinydir_close(&dir);
            return false;
        }

        //printf("%s", file.name);
        if (!file.is_dir)
        {
            const std::string   strFilename = strDicomDir + '\\' + std::string(file.name);

            if (!parser.OpenFile(dicom_stl::string(strFilename.c_str())))
            {
                std::cout << "Couldn't open " << strFilename << "\n";
                return false;
            }

            if (parser.IsDICOMFile())
            {
                if (!parser.ReadHeader())
                {
                    std::cout << "Couldn't read dicom header\nPress any key\n";
                    return false;
                }

                DICOMParser::VRTypes    dataType;
                unsigned long           len = 0;

                helper.GetImageData( pvBuffer, dataType, len );

                float * fPos = helper.GetImagePositionPatient();
                vfZ.push_back(fPos[2]);

                uiNumSlices++;
            }
        }

        if (tinydir_next(&dir) == -1)
        {
            perror("Error getting next file");
            tinydir_close(&dir);
            return false;
        }
    }

    tinydir_close(&dir);

    float * fvSpacing = helper.GetPixelSpacing();

    fXSpacing = fvSpacing[0];
    fYSpacing = fvSpacing[1];

    std::sort(vfZ.begin(), vfZ.end());
    fZSpacing = vfZ[1] - vfZ[0];

    return true;
}

bool GetDicom3DBuffer(  const std::string &     strDicomDir,
                        DICOMParser &           parser,
                        DICOMAppHelper &        helper,
                        int16_t *               piBuffer,
                        const uint32_t          uiNumSlices )
{
    tinydir_dir dir;
    if (tinydir_open(&dir, strDicomDir.c_str()) == -1)
    {
        perror("Error opening dicom dir\n");
        return false;
    }

    uint32_t    uiNumSlicesCount = 0;
    int16_t *   piDest = piBuffer;

    const uint32_t    uiPixelSize = helper.GetWidth() * helper.GetHeight();
    const uint32_t    ui2DBufferSize = uiPixelSize * sizeof(int16_t);

    while (dir.has_next)
    {
        tinydir_file file;
        if (tinydir_readfile(&dir, &file) == -1)
        {
            perror("Error getting file\n");
            tinydir_close(&dir);
            return false;
        }

        // printf("%s\n", file.name);
        if (!file.is_dir)
        {
            const std::string   strFilename = strDicomDir + '\\' + std::string(file.name);

            if (!parser.OpenFile(dicom_stl::string(strFilename.c_str())))
            {
                std::cout << "Couldn't open " << strFilename << "\n";
                return false;
            }

            if (parser.IsDICOMFile())
            {
                if (!parser.ReadHeader())
                {
                    std::cout << "Couldn't read dicom header\nPress any key\n";
                    return false;
                }

                DICOMParser::VRTypes    dataType;
                unsigned long           len = 0;

                void *      pvBuffer = NULL;

                helper.GetImageData(pvBuffer, dataType, len);

                uiNumSlicesCount++;
                assert(uiNumSlicesCount <= uiNumSlices);

                memcpy( reinterpret_cast<void *>(piDest), pvBuffer, ui2DBufferSize );
                piDest += uiPixelSize;
            }
        }

        if (tinydir_next(&dir) == -1)
        {
            perror("Error getting next file");
            tinydir_close(&dir);
            return false;
        }
    }

    tinydir_close(&dir);

    return true;
}

int16_t nearestVoxel(   const int16_t * pSrc,
                        const float     fX,
                        const float     fY,
                        const float     fZ,
                        const int       iSizeX,
                        const int       iSizeY,
                        const int       iSizeZ    )
{
    int     iX      = int(fX);
    int     iY      = int(fY);
    int     iZ      = int(fZ);
    int     iSlice  = iSizeX * iSizeY * iZ;
    int     iRow    = iSizeX * iY;

    return pSrc[iSlice + iRow + iX];
}

// http://paulbourke.net/miscellaneous/interpolation/

int16_t trilinearVoxel( const int16_t * pSrc,
                        const float     fX,
                        const float     fY,
                        const float     fZ,
                        const int       iSizeX,
                        const int       iSizeY,
                        const int       iSizeZ  )
{

    int     iX0 = int(fX);
    int     iY0 = int(fY);
    int     iZ0 = int(fZ);
    float   fX0 = float(iX0);
    float   fY0 = float(iY0);
    float   fZ0 = float(iZ0);
    int     iX1 = min(iX0 + 1, iSizeX - 1);
    int     iY1 = min(iY0 + 1, iSizeY - 1);
    int     iZ1 = min(iZ0 + 1, iSizeZ - 1);
    float   fX1 = float(iX1);
    float   fY1 = float(iY1);
    float   fZ1 = float(iZ1);
    float   V000 = float(nearestVoxel(pSrc, fX0, fY0, fZ0, iSizeX, iSizeY, iSizeZ));
    float   V001 = float(nearestVoxel(pSrc, fX0, fY0, fZ1, iSizeX, iSizeY, iSizeZ));
    float   V010 = float(nearestVoxel(pSrc, fX0, fY1, fZ0, iSizeX, iSizeY, iSizeZ));
    float   V011 = float(nearestVoxel(pSrc, fX0, fY1, fZ1, iSizeX, iSizeY, iSizeZ));
    float   V100 = float(nearestVoxel(pSrc, fX1, fY0, fZ0, iSizeX, iSizeY, iSizeZ));
    float   V101 = float(nearestVoxel(pSrc, fX1, fY0, fZ1, iSizeX, iSizeY, iSizeZ));
    float   V110 = float(nearestVoxel(pSrc, fX1, fY1, fZ0, iSizeX, iSizeY, iSizeZ));
    float   V111 = float(nearestVoxel(pSrc, fX1, fY1, fZ1, iSizeX, iSizeY, iSizeZ));
    float   x = fX - fX0;
    float   y = fY - fY0;
    float   z = fZ - fZ0;
    float   Vxyz =  (V000*(1 - x)*(1 - y)*(1 - z)) +
                    (V100*x*(1 - y)*(1 - z)) +
                    (V010*(1 - x)*y*(1 - z)) +
                    (V001*(1 - x)*(1 - y)*z) +
                    (V101*x*(1 - y)*z) +
                    (V011*(1 - x)*y*z) +
                    (V110*x*y*(1 - z)) +
                    (V111*x*y*z);

    return int16_t(Vxyz);
}

void ResampleBuffer(    const int16_t * pSrc,
                        int16_t **      ppDest,
                        const int &     iSrcSizeX,
                        const int &     iSrcSizeY,
                        const int &     iSrcSizeZ,
                        const float &   fXSpacing,
                        const float &   fYSpacing,
                        const float &   fZSpacing,
                        int &           iDestSizeX,
                        int &           iDestSizeY,
                        int &           iDestSizeZ  )
{
    iDestSizeX = iSrcSizeX;
    iDestSizeY = iSrcSizeY;
    iDestSizeZ = int(float(iSrcSizeZ) * fZSpacing / fXSpacing);

    const int   iDestSize = iDestSizeX * iDestSizeY * iDestSizeZ;

    *ppDest = new int16_t[iDestSize];

    int16_t *      pDest = *ppDest;

    float   fSX = float(iSrcSizeX) / float(iDestSizeX);
    float   fSY = float(iSrcSizeY) / float(iDestSizeY);
    float   fSZ = float(iSrcSizeZ) / float(iDestSizeZ);

    for (int iZ = 0; iZ < iDestSizeZ; iZ++)
    {
        float   fZ = float(iZ) * fSZ;

        for (int iY = 0; iY < iDestSizeY; iY++)
        {
            float   fY = float(iY) * fSY;

            for (int iX = 0; iX < iDestSizeX; iX++)
            {
                float   fX = float(iX) * fSX;
                //*pDest++ = nearestVoxel(pSrc, fX, fY, fZ, iSrcSizeX, iSrcSizeY, iSrcSizeZ);
                *pDest++ = trilinearVoxel(pSrc, fX, fY, fZ, iSrcSizeX, iSrcSizeY, iSrcSizeZ);
            }
        }
    }
}

void ReadDir( const std::string & strDir)
{
    DICOMParser     parser;
    DICOMAppHelper  helper;

    helper.RegisterCallbacks(&parser);
    helper.RegisterPixelDataCallback(&parser);

    void *      pvBuffer = NULL;
    uint32_t    uiNumSlices = 0;
    float       fXSpacing = 0.0f;
    float       fYSpacing = 0.0f;
    float       fZSpacing = 0.0f;
    bool        bOK = GetDicomDirDimensions(    strDir,
                                                parser,
                                                helper,
                                                pvBuffer,
                                                uiNumSlices,
                                                fXSpacing,
                                                fYSpacing,
                                                fZSpacing   );

    std::cout << "Spacing = ( " << fXSpacing << ", " << fYSpacing << ", " << fZSpacing << "\n";

    int         iSize = helper.GetWidth() *
                        helper.GetHeight() *
                        uiNumSlices *
                        sizeof(int16_t);
    int16_t *   piBufferSrc = new int16_t[iSize];

    bOK = GetDicom3DBuffer( strDir,
                            parser,
                            helper,
                            piBufferSrc,
                            uiNumSlices);

    int16_t *   piBufferDest = new int16_t[iSize];
    int         iDestSizeX = 0;
    int         iDestSizeY = 0;
    int         iDestSizeZ = 0;

    ResampleBuffer( piBufferSrc,
                    &piBufferDest,
                    helper.GetWidth(),
                    helper.GetHeight(),
                    uiNumSlices,
                    fXSpacing,
                    fYSpacing,
                    fZSpacing,
                    iDestSizeX,
                    iDestSizeY,
                    iDestSizeZ);


    bOK = SaveVTK(  "test1.vtk",
                    piBufferSrc,
                    helper.GetWidth(),
                    helper.GetHeight(),
                    uiNumSlices,
                    1.0f);

    bOK = SaveVTK(  "test2.vtk",
                    piBufferDest,
                    iDestSizeX,
                    iDestSizeY,
                    iDestSizeZ,
                    1.0f        );

    int16_t iMin = INT16_MAX;
    int16_t iMax = INT16_MIN;
    for (int i = 0; i < iSize; i++)
    {
        iMin = min(iMin, piBufferSrc[i]);
        iMax = max(iMax, piBufferSrc[i]);
    }
    std::cout << "Min = " << iMin << " Max = " << iMax << "\n";

    delete[] piBufferSrc;
    delete[] piBufferDest;
}

void main( int argc, char **argv )
{
    if (argc != 2)
    {
        std::cout << "Use : DicomReader [DICOM directory]\nPress any key\n";
        std::cin.ignore();
        return;
    }

    ReadDir( std::string( argv[1] ) );

    std::cout << "Press any key\n";
    std::cin.ignore();
}