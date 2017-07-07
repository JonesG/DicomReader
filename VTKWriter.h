#pragma once

#include <fstream>
#include <string>

bool WriteVTK(  const std::string & strFileName,
                const int16_t *     pVoxels,
                const uint32_t      iX,
                const uint32_t      iY,
                const uint32_t      iZ,
                const float         fCellSize )
{
    bool ascii = false;

    std::ofstream vtkstream;

    if (ascii)
    {
        vtkstream.open(strFileName, std::ios::out);
    }
    else
    {
        vtkstream.open(strFileName, std::ios::out | std::ios::binary);
    }

    if (!vtkstream)
    {
        return false;
    }

    vtkstream << "# vtk DataFile Version 2.0" << "\n";
    vtkstream << "Example" << "\n";

    if (ascii)
    {
        vtkstream << "ASCII" << "\n";
    }
    else
    {
        vtkstream << "BINARY" << "\n";
    }

    vtkstream << "DATASET STRUCTURED_POINTS" << std::endl;
    vtkstream << "DIMENSIONS " << iX << " " << iY << " " << iZ << std::endl;
    vtkstream << "ASPECT_RATIO " << fCellSize << " " << fCellSize << " " << fCellSize << std::endl;
    vtkstream << "ORIGIN " << 0.0f << " " << 0.0f << " " << 0.0f << std::endl;
    vtkstream << "POINT_DATA " << iX*iY*iZ << std::endl;
    vtkstream << "SCALARS volume_scalars short 1" << std::endl;
    vtkstream << "LOOKUP_TABLE default" << std::endl;

    if (ascii)
    {
        for (unsigned int i = 0; i<iX*iY*iZ; ++i)
        {
            vtkstream << pVoxels[i] << " ";
        }
    }
    else
    {
        vtkstream.write( reinterpret_cast<char *>(const_cast<int16_t *>(pVoxels)), iX*iY*iZ*sizeof(int16_t) );
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
        vtkstream.read(reinterpret_cast<char *>(const_cast<int16_t *>(*ppVoxels)), iX*iY*iZ * sizeof(int16_t));
    }

    // read final character to ensure eof
    vtkstream.read(reinterpret_cast<char *>(&uc1), sizeof(int8_t));
    assert( vtkstream.eof() );

    vtkstream.close();

    return true;
}
