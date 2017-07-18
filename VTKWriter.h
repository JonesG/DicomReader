#pragma once

#include <fstream>
#include <string>

#undef min
#undef max

#include <algorithm>

#include <base64.h>

bool WriteVTKHeader(    std::ofstream &     vtkstream,
                        const std::string & strFileName,
                        const int16_t *     pVoxels,
                        const uint32_t      iX,
                        const uint32_t      iY,
                        const uint32_t      iZ,
                        const float         fXSpacing,
                        const float         fYSpacing,
                        const float         fZSpacing       )
{
    vtkstream.open(strFileName, std::ios::out | std::ios::binary);
    if (!vtkstream)
    {
        return false;
    }

    vtkstream << "# vtk DataFile Version 2.0" << "\n";
    vtkstream << "Example" << "\n";
    vtkstream << "BINARY" << "\n";
    vtkstream << "DATASET STRUCTURED_POINTS" << std::endl;
    vtkstream << "DIMENSIONS " << iX << " " << iY << " " << iZ << std::endl;
    vtkstream << "ASPECT_RATIO " << fXSpacing << " " << fYSpacing << " " << fZSpacing << std::endl;
    vtkstream << "ORIGIN " << 0.0f << " " << 0.0f << " " << 0.0f << std::endl;
    vtkstream << "POINT_DATA " << iX*iY*iZ << std::endl;
    vtkstream << "SCALARS volume_scalars short 1" << std::endl;
    vtkstream << "LOOKUP_TABLE default" << std::endl;

    return true;
}

bool WriteVTK(  const std::string & strFileName,
                const int16_t *     pVoxels,
                const uint32_t      iX,
                const uint32_t      iY,
                const uint32_t      iZ,
                const float         fXSpacing,
                const float         fYSpacing,
                const float         fZSpacing   )
{
    std::ofstream vtkstream;

    if ( !WriteVTKHeader(   vtkstream,
                            strFileName,
                            pVoxels,
                            iX,
                            iY,
                            iZ,
                            fXSpacing,
                            fYSpacing,
                            fZSpacing)  )
    {
        return false;
    }

    //vtkstream.write( reinterpret_cast<char *>(const_cast<int16_t *>(pVoxels)), iX*iY*iZ*sizeof(int16_t) );

    uint8_t * pu1 = reinterpret_cast<uint8_t *>(const_cast<int16_t *>(pVoxels));
    for (uint32_t i = 0; i < iX*iY*iZ; i++)
    {
        // endian swap
        vtkstream.write( reinterpret_cast<char *>(pu1+1), 1 );
        vtkstream.write( reinterpret_cast<char *>(pu1), 1 );
        pu1 += 2;
    }

    vtkstream.close();

    return true;
}

bool WriteVTK(  const std::string & strFileName,
                const int16_t *     pVoxels,
                const uint32_t      iX,
                const uint32_t      iY,
                const uint32_t      iZ,
                const uint32_t      iOriginX,
                const uint32_t      iOriginY,
                const uint32_t      iOriginZ,
                const uint32_t      iPitchX,
                const uint32_t      iPitchY,
                const uint32_t      iPitchZ,
                const float         fXSpacing,
                const float         fYSpacing,
                const float         fZSpacing   )
{
    std::ofstream vtkstream;

    if ( !WriteVTKHeader(   vtkstream,
                            strFileName,
                            pVoxels,
                            iX,
                            iY,
                            iZ,
                            fXSpacing,
                            fYSpacing,
                            fZSpacing))
    {
        return false;
    }

    //vtkstream.write( reinterpret_cast<char *>(const_cast<int16_t *>(pVoxels)), iX*iY*iZ*sizeof(int16_t) );

    uint32_t    iEndX = std::min(iPitchX, iOriginX + iX);
    uint32_t    iEndY = std::min(iPitchY, iOriginY + iY);
    uint32_t    iEndZ = std::min(iPitchZ, iOriginZ + iZ);

    for ( uint32_t i = iOriginZ; i < iEndZ; i++ )
    {
        uint32_t    iZStart = i * iPitchX * iPitchY;

        for (uint32_t j = iOriginY; j < iEndY; j++)
        {
            uint32_t    iYStart = j * iPitchX;

            const int16_t * pStart = &pVoxels[iZStart+iYStart+ iOriginX];
            uint8_t *       pu1 = reinterpret_cast<uint8_t *>(const_cast<int16_t *>(pStart));

            for (uint32_t k = iOriginX; k < iEndX; k++)
            {
                // endian swap
                vtkstream.write(reinterpret_cast<char *>(pu1 + 1), 1);
                vtkstream.write(reinterpret_cast<char *>(pu1), 1);
                pu1 += 2;
            }
        }
    }

    vtkstream.close();

    return true;
}

bool ReadVTK(   const std::string & strFileName,
                int16_t **          ppVoxels,
                uint32_t &          iX,
                uint32_t &          iY,
                uint32_t &          iZ,
                float    &          fCellSize   )
{
    bool ascii = false;

    std::ifstream vtkstream;

    if (ascii)
    {
        vtkstream.open(strFileName, std::ios::in);
    }
    else
    {
        vtkstream.open(strFileName, std::ios::in | std::ios::binary);
    }

    if (!vtkstream)
    {
        return false;
    }

    std::string str1;
    std::string str2;
    std::string str3;
    std::string str4;
    std::string str5;
    // "# vtk DataFile Version 2.0"
    vtkstream >> str1 >> str2 >> str3 >> str4 >> str5;
    // "Example"
    vtkstream >> str1;

    // "ASCII" or "BINARY"
    vtkstream >> str1;

    if ( !(( ascii && (str1 == "ASCII") ) ||
           (!ascii && (str1 == "BINARY")))   )
    {
        return false;
    }

    // "DATASET STRUCTURED_POINTS"
    vtkstream >> str1 >> str2;

    // "DIMENSIONS " << iX << " " << iY << " " << iZ;
    vtkstream >> str1 >> iX >> iY >> iZ;

    //"ASPECT_RATIO " << fCellSize << " " << fCellSize << " " << fCellSize;
    vtkstream >> str1 >> fCellSize >> fCellSize >> fCellSize;

    //"ORIGIN " << x << " " << y << " " << z;
    vtkstream >> str1 >> str2 >> str3 >> str4;

    //"POINT_DATA " << iX*iY*iZ;
    uint32_t    uiPointDataSize;
    vtkstream >> str1 >> uiPointDataSize;
    assert( uiPointDataSize == iX*iY*iZ );

    //"SCALARS volume_scalars short 1";
    vtkstream >> str1 >> str2 >> str3 >> str4;

    //"LOOKUP_TABLE default";
    vtkstream >> str1 >> str2;
    
    // eol character before point data
    int8_t uc1;
    vtkstream.read(reinterpret_cast<char *>(&uc1), sizeof(int8_t));

    *ppVoxels = new int16_t[uiPointDataSize];

    if (ascii)
    {
        for (unsigned int i = 0; i<uiPointDataSize; ++i)
        {
            vtkstream >> *ppVoxels[i];
        }
    }
    else
    {
        //vtkstream.read(reinterpret_cast<char *>(const_cast<int16_t *>(*ppVoxels)), iX*iY*iZ * sizeof(int16_t));
        uint8_t * pu1 = reinterpret_cast<uint8_t *>(const_cast<int16_t *>(*ppVoxels));
        for (uint32_t i = 0; i < iX*iY*iZ; i++)
        {
            // endian swap
            vtkstream.read(reinterpret_cast<char *>(pu1 + 1), 1);
            vtkstream.read(reinterpret_cast<char *>(pu1), 1);
            pu1 += 2;
        }
    }

    // read final character to ensure eof
    vtkstream.read(reinterpret_cast<char *>(&uc1), sizeof(int8_t));
    assert( vtkstream.eof() );

    vtkstream.close();

    return true;
}

bool WriteVTU(   const std::string & strFileName,
                    const int16_t *     pVoxels,
                    const uint32_t      iX,
                    const uint32_t      iY,
                    const uint32_t      iZ,
                    const float         fXSpacing,
                    const float         fYSpacing,
                    const float         fZSpacing   )
{
    std::ofstream vtkstream;

    vtkstream.open(strFileName, std::ios::out | std::ios::binary);

    if (!vtkstream)
    {
        return false;
    }

    std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(pVoxels), iX*iY*iZ * sizeof(int16_t));

    vtkstream << "<VTKFile type = \"ImageData\" version = \"1.0\" byte_order = \"LittleEndian\" header_type = \"UInt64\">\n";
    vtkstream << "<ImageData WholeExtent = \"0 " << iX-1 << " 0 " << iY-1 << " 0 " << iZ-1 << "\"";
    vtkstream << " Origin = \"0 0 0\"";
    vtkstream << " Spacing = \"" << fXSpacing << " " << fYSpacing << " " << fZSpacing << "\">\n";
    vtkstream << "<Piece Extent = \"0 " << iX-1 << " 0 " << iY-1 << " 0 " << iZ-1 << "\">\n";
    vtkstream << "<PointData Scalars = \"my_scalars\">\n";
    vtkstream << "<DataArray type = \"Int16\" Name = \"volume_scalars\" format = \"binary\">\n";
    vtkstream << encoded;
    vtkstream << "</DataArray>\n";
    vtkstream << "</PointData>\n";
    vtkstream << "<CellData></CellData>\n";
    vtkstream << "</Piece>\n";
    vtkstream << "</ImageData>\n";

    vtkstream << "</VTKFile>\n";

    vtkstream.close();

    return true;
}
