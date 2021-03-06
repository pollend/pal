/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2015-2018 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 **********************************************************************************************************************/

#pragma once

#include "pal.h"

namespace Pal
{

class      CmdStream;
class      Device;
class      GfxCmdBuffer;
class      GpuMemory;
class      Image;
struct     BufferViewInfo;
struct     ImageViewInfo;
struct     SubresRange;
enum class RpmComputePipeline : uint32;

namespace RpmUtil
{

// Returns the minimum number of thread groups needed to launch at least minThreads.
PAL_INLINE uint32 MinThreadGroups(uint32 minThreads, uint32 threadsPerGroup)
    { return (minThreads + threadsPerGroup - 1) / threadsPerGroup; }

extern void BuildRawBufferViewInfo(BufferViewInfo* pInfo, const GpuMemory& bufferMemory, gpusize byteOffset);

extern void BuildImageViewInfo(
    ImageViewInfo*     pInfo,
    const Image&       image,
    const SubresRange& subresRange,
    SwizzledFormat     swizzledFormat,
    bool               isShaderWriteable,
    ImageTexOptLevel   texOptLevel);

extern SwizzledFormat GetRawFormat(ChNumFormat format, uint32* pTexelScale);

extern uint32* CreateAndBindEmbeddedUserData(
    GfxCmdBuffer*     pCmdBuffer,
    uint32            sizeInDwords,
    uint32            alignmentInDwords,
    PipelineBindPoint bindPoint,
    uint32            entryToBind);

extern void ConvertClearColorToNativeFormat(
    SwizzledFormat baseFormat,
    SwizzledFormat clearFormat,
    uint32*        pColor);

extern uint32 GetNormalizedData(uint32 inputData, uint32 maxComponentValue);

extern void WriteVsZOut(GfxCmdBuffer* pCmdBuffer, float depthValue);
extern void WriteVsFirstSliceOffet(GfxCmdBuffer* pCmdBuffer, uint32 firstSliceIndex);
extern void BindBltRasterState(GfxCmdBuffer* pCmdBuffer);

// Helper structure containing the constant buffer data for CmdCopyImage blits.
struct CopyImageInfo
{
    Offset3d srcOffset;             // srcOffset.z is either Src Z Offset (3D) or slice (1D/2D).
    uint32   numSamples;            // Sample count (for gamma-correction shader ONLY)
    Offset3d dstOffset;             // dstOffset.z is either Dst Z Offset (3D) or slice (1D/2D).
    struct
    {
        uint32 srcMipLevel : 16;
        uint32 dstMipLevel : 16;
    } packedMipData;                // packed source and dest mipmap levels (for the mip-level shader ONLY).
    Extent2d copyRegion;
};

// Size of a CopyImageInfo structure, in DWORD's.
constexpr uint32 CopyImageInfoDwords = ((sizeof(CopyImageInfo) + (sizeof(uint32) - 1)) / sizeof(uint32));

// Helper structure containing the constant buffer data for YUV-to-RGB conversion blits.
struct YuvRgbConversionInfo
{
    float     srcLeft;              // Left of the source copy region (normalized coordinates)
    float     srcTop;               // Top of the source copy region (normalized coordinates)
    float     srcRight;             // Right of the source copy region (normalized coordinates)
    float     srcBottom;            // Bottom of the source copy region (normalized coordinates)
    Offset2d  dstOffset;            // Offset into the destination to begin the copy
    Extent2d  dstExtent;            // Region of the destination which will be copied into
    bool      gammaCorrection;      // Toggles gamma correction for the destination
    float     srcWidthEpsilon;      // Distance between two pixels in a macro-pixel source (normalized coordinates)
    bool      reversePacking;       // Reverses the packing order in a macro-pixel destination
    uint32    unused;
    float     cscTable[3][4];       // Color-space-conversion table (4x3 matrix)
};

// Size of a YuvToRgbCopyInfo structure, in DWORD's.
constexpr uint32 YuvRgbConversionInfoDwords = ((sizeof(YuvRgbConversionInfo) + (sizeof(uint32) - 1)) / sizeof(uint32));

// Helper struct for setting up the Pipeline and Image view info for each component (Y, Cb, Cr) of a YUV image for
// color-space-conversion blits.
struct ColorSpaceConversionInfo
{
    RpmComputePipeline  pipelineYuvToRgb;
    struct
    {
        ImageAspect     aspect;
        SwizzledFormat  swizzledFormat;
    } viewInfoYuvToRgb[3];

    RpmComputePipeline  pipelineRgbToYuv;
    struct
    {
        ImageAspect     aspect;
        SwizzledFormat  swizzledFormat;
        // Note: These indices represent which order the rows of the color-space-conversion matrix should be swizzled
        // in order for the conversion to work properly. See SetupRgbToYuvCscTable() for more details.
        uint16          matrixRowOrder[3];
    } viewInfoRgbToYuv[3];
};

// Number of YUV formats.
constexpr uint32 YuvFormatCount = (static_cast<uint32>(ChNumFormat::Count) - static_cast<uint32>(ChNumFormat::AYUV));

// Lookup table containing the Pipeline and Image-View information for each (Y,Cb,Cr) component of a YUV image when
// doing color-space-conversion blits.
extern const ColorSpaceConversionInfo CscInfoTable[YuvFormatCount];

// Helper function for setting up the color space conversion table for each pass of an RGB to YUV color-space
// conversion blit.
extern void SetupRgbToYuvCscTable(
    ChNumFormat                      format,
    uint32                           pass,
    const ColorSpaceConversionTable& cscTable,
    YuvRgbConversionInfo*            pInfo);

//Helper function to calculate how many bits are required to to represent each sample of the fmask
extern uint32 CalculatNumFmaskBits(uint32 fragments, uint32 samples);

} // RpmUtil
} // Pal
