/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2014-2019 Advanced Micro Devices, Inc. All Rights Reserved.
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

#include "core/hw/gfxip/computePipeline.h"
#include "core/hw/gfxip/gfx6/gfx6Chip.h"

namespace Pal
{

class Platform;

namespace Gfx6
{

class ComputePipelineUploader;
class Device;

// =====================================================================================================================
// GFX6 compute pipeline class: implements GFX6 specific functionality for the ComputePipeline class.
class ComputePipeline : public Pal::ComputePipeline
{
public:
    ComputePipeline(Device* pDevice, bool isInternal);
    virtual ~ComputePipeline() { }

    uint32* WriteCommands(
        Pal::CmdStream*                 pCmdStream,
        uint32*                         pCmdSpace,
        const DynamicComputeShaderInfo& csInfo,
        bool                            prefetch) const;

    virtual Result GetShaderStats(
        ShaderType   shaderType,
        ShaderStats* pShaderStats,
        bool         getDisassemblySize) const override;

    const ComputePipelineSignature& Signature() const { return m_signature; }

protected:
    virtual Result HwlInit(
        const AbiProcessor&              abiProcessor,
        const CodeObjectMetadata&        metadata,
#if PAL_CLIENT_INTERFACE_MAJOR_VERSION >= 440
        ComputePipelineIndirectFuncInfo* pIndirectFuncList,
        uint32                           indirectFuncCount,
#endif
        Util::MsgPackReader*             pMetadataReader) override;

private:
    uint32 CalcMaxWavesPerSh(uint32 maxWavesPerCu) const;

    void BuildPm4Headers(const ComputePipelineUploader& uploader);
    void UpdateRingSizes(const CodeObjectMetadata& metadata);

    void SetupSignatureFromElf(
        const CodeObjectMetadata& metadata,
        const RegisterVector&     registers);

    Device*const  m_pDevice;

    // Pre-assembled "images" of the PM4 packets used for binding this pipeline to a command buffer.
    struct Pm4Commands
    {
        struct
        {
            PM4CMDLOADDATAINDEX  loadShRegIndex;
        } loadIndex; // LOAD_INDEX path, used for universal command buffers on updated microcode.

        struct
        {
            PM4CMDSETDATA            hdrComputeNumThread;
            regCOMPUTE_NUM_THREAD_X  computeNumThreadX;
            regCOMPUTE_NUM_THREAD_Y  computeNumThreadY;
            regCOMPUTE_NUM_THREAD_Z  computeNumThreadZ;

            PM4CMDSETDATA      hdrComputePgm;
            regCOMPUTE_PGM_LO  computePgmLo;
            regCOMPUTE_PGM_HI  computePgmHi;

            PM4CMDSETDATA         hdrComputePgmRsrc1;
            regCOMPUTE_PGM_RSRC1  computePgmRsrc1;

            PM4CMDSETDATA           hdrComputeUserData;
            regCOMPUTE_USER_DATA_0  computeUserDataLo;
        } set; // SET path, used for compute command buffers & legacy microcode.  The MEC doesn't support
               // LOAD_SH_REG_INDEX.

        struct
        {
            PM4CMDSETDATA         hdrComputePgmRsrc2;
            regCOMPUTE_PGM_RSRC2  computePgmRsrc2;

            PM4CMDSETDATA               hdrComputeResourceLimits;
            regCOMPUTE_RESOURCE_LIMITS  computeResourceLimits;
        } dynamic; // Contains state which depends on bind-time parameters.

        PipelinePrefetchPm4 prefetch;
    };

    ComputePipelineSignature     m_signature;
    Pm4Commands                  m_commands;

    PAL_DISALLOW_DEFAULT_CTOR(ComputePipeline);
    PAL_DISALLOW_COPY_AND_ASSIGN(ComputePipeline);
};

// =====================================================================================================================
// Extension of the PipelineUploader helper class for Gfx6/7/8 compute pipelines.
class ComputePipelineUploader : public Pal::PipelineUploader
{
public:
    explicit ComputePipelineUploader(
        uint32 shRegisterCount)
        :
        PipelineUploader(0, shRegisterCount)
        { }
    virtual ~ComputePipelineUploader() { }

    // Add a SH register to GPU memory for use with IT_LOAD_SH_REG_INDEX.
    template <typename Register_t>
    PAL_INLINE void AddShReg(uint16 address, Register_t reg)
        { Pal::PipelineUploader::AddShRegister(address - PERSISTENT_SPACE_START, reg.u32All); }

private:
    PAL_DISALLOW_DEFAULT_CTOR(ComputePipelineUploader);
    PAL_DISALLOW_COPY_AND_ASSIGN(ComputePipelineUploader);
};

} // Gfx6
} // Pal
