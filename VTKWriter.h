#pragma once

#include <fstream>
#include <string>

bool SaveVTK(   const std::string & strFileName,
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
