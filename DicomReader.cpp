#include <iostream>

#include "DICOMParser.h"
#include "DICOMAppHelper.h"

#include "tinydir.h"

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
       char cVal = static_cast<char>(((iVal - iMin) * 255) / iScale);
       iMinP = min(iMinP, int(cVal));
       iMaxP = max(iMaxP, int(cVal));
       cVal += 127;
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
                    DICOMAppHelper &        helper  )
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

    void *                  pvBuffer = NULL;
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

bool ReadDicomDir( const std::string & strDicomDir )
{
    DICOMParser     parser;
    DICOMAppHelper  helper;

    helper.RegisterCallbacks(&parser);
    helper.RegisterPixelDataCallback(&parser);

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

        printf("%s", file.name);
        if (file.is_dir)
        {
            printf("/");
        }
        else
        {
            bool    bOk = ReadDicomFile(strDicomDir + '\\' + std::string(file.name), parser, helper);
        }
        printf("\n");

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

void main( int argc, char **argv )
{
    if (argc != 2)
    {
        std::cout << "Use : DicomReader [DICOM directory]\nPress any key\n";
        std::cin.ignore();
        return;
    }

    ReadDicomDir( std::string( argv[1] ) );

    std::cout << "Press any key\n";
    std::cin.ignore();
}