/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2019 Advanced Micro Devices, Inc. All Rights Reserved.
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

#include "protocols/rgpClient.h"
#include "msgChannel.h"

#include <string.h>

#define RGP_CLIENT_MIN_MAJOR_VERSION 2
#define RGP_CLIENT_MAX_MAJOR_VERSION 8

namespace DevDriver
{
    namespace RGPProtocol
    {
        RGPClient::RGPClient(IMsgChannel* pMsgChannel)
            : BaseProtocolClient(pMsgChannel, Protocol::RGP, RGP_CLIENT_MIN_MAJOR_VERSION, RGP_CLIENT_MAX_MAJOR_VERSION)
        {
            memset(&m_traceContext, 0, sizeof(m_traceContext));
        }

        RGPClient::~RGPClient()
        {
        }

        Result RGPClient::BeginTrace(const BeginTraceInfo& traceInfo)
        {
            Result result = Result::Error;

            if ((m_traceContext.state == TraceState::Idle)         &&
                (traceInfo.callbackInfo.chunkCallback != nullptr))
            {
                RGPPayload payload = {};
                payload.command = RGPMessage::ExecuteTraceRequest;

                if (GetSessionVersion() < RGP_PROFILING_CLOCK_MODES_VERSION)
                {
                    payload.executeTraceRequest.parameters.gpuMemoryLimitInMb = traceInfo.parameters.gpuMemoryLimitInMb;
                    payload.executeTraceRequest.parameters.numPreparationFrames = traceInfo.parameters.numPreparationFrames;
                    payload.executeTraceRequest.parameters.flags.u32All = traceInfo.parameters.flags.u32All;
                }
                else if ((GetSessionVersion() == RGP_PROFILING_CLOCK_MODES_VERSION) ||
                         (GetSessionVersion() == RGP_TRACE_PROGRESS_VERSION))
                {
                    payload.executeTraceRequestV2.parameters.gpuMemoryLimitInMb = traceInfo.parameters.gpuMemoryLimitInMb;
                    payload.executeTraceRequestV2.parameters.numPreparationFrames = traceInfo.parameters.numPreparationFrames;
                    payload.executeTraceRequestV2.parameters.clockMode = ProfilingClockMode::Stable;
                    payload.executeTraceRequestV2.parameters.flags.u32All = traceInfo.parameters.flags.u32All;
                }
                else if (GetSessionVersion() == RGP_COMPUTE_PRESENTS_VERSION)
                {
                    payload.executeTraceRequestV3.parameters.gpuMemoryLimitInMb = traceInfo.parameters.gpuMemoryLimitInMb;
                    payload.executeTraceRequestV3.parameters.numPreparationFrames = traceInfo.parameters.numPreparationFrames;
                    payload.executeTraceRequestV3.parameters.flags.u32All = traceInfo.parameters.flags.u32All;
                }
                else if ((GetSessionVersion() == RGP_TRIGGER_MARKERS_VERSION) ||
                         (GetSessionVersion() == RGP_PENDING_ABORT_VERSION))
                {
                    payload.executeTraceRequestV4.parameters.gpuMemoryLimitInMb = traceInfo.parameters.gpuMemoryLimitInMb;
                    payload.executeTraceRequestV4.parameters.numPreparationFrames = traceInfo.parameters.numPreparationFrames;
                    payload.executeTraceRequestV4.parameters.flags.u32All = traceInfo.parameters.flags.u32All;

                    payload.executeTraceRequestV4.parameters.beginTagLow =
                        static_cast<uint32>(traceInfo.parameters.beginTag & 0xFFFFFFFF);
                    payload.executeTraceRequestV4.parameters.beginTagHigh =
                        static_cast<uint32>((traceInfo.parameters.beginTag >> 32) & 0xFFFFFFFF);

                    payload.executeTraceRequestV4.parameters.endTagLow =
                        static_cast<uint32>(traceInfo.parameters.endTag & 0xFFFFFFFF);
                    payload.executeTraceRequestV4.parameters.endTagHigh =
                        static_cast<uint32>((traceInfo.parameters.endTag >> 32) & 0xFFFFFFFF);

                    Platform::Strncpy(payload.executeTraceRequestV4.parameters.beginMarker,
                                      traceInfo.parameters.beginMarker,
                                      sizeof(payload.executeTraceRequestV4.parameters.beginMarker));

                    Platform::Strncpy(payload.executeTraceRequestV4.parameters.endMarker,
                                      traceInfo.parameters.endMarker,
                                      sizeof(payload.executeTraceRequestV4.parameters.endMarker));
                }
                else if (GetSessionVersion() >= RGP_FRAME_CAPTURE_VERSION)
                {
                    payload.executeTraceRequestV5.parameters.gpuMemoryLimitInMb = traceInfo.parameters.gpuMemoryLimitInMb;
                    payload.executeTraceRequestV5.parameters.numPreparationFrames = traceInfo.parameters.numPreparationFrames;
                    payload.executeTraceRequestV5.parameters.captureMode = traceInfo.parameters.captureMode;
                    payload.executeTraceRequestV5.parameters.flags.u32All = traceInfo.parameters.flags.u32All;

                    if (traceInfo.parameters.captureMode == CaptureTriggerMode::Index)
                    {
                        payload.executeTraceRequestV5.parameters.captureStartIndex = traceInfo.parameters.captureStartIndex;
                        payload.executeTraceRequestV5.parameters.captureStopIndex  = traceInfo.parameters.captureStopIndex;
                    }
                    else if (traceInfo.parameters.captureMode == CaptureTriggerMode::Markers)
                    {
                        payload.executeTraceRequestV5.parameters.beginTagLow =
                            static_cast<uint32>(traceInfo.parameters.beginTag & 0xFFFFFFFF);
                        payload.executeTraceRequestV5.parameters.beginTagHigh =
                            static_cast<uint32>((traceInfo.parameters.beginTag >> 32) & 0xFFFFFFFF);

                        payload.executeTraceRequestV5.parameters.endTagLow =
                            static_cast<uint32>(traceInfo.parameters.endTag & 0xFFFFFFFF);
                        payload.executeTraceRequestV5.parameters.endTagHigh =
                            static_cast<uint32>((traceInfo.parameters.endTag >> 32) & 0xFFFFFFFF);

                        Platform::Strncpy(payload.executeTraceRequestV5.parameters.beginMarker,
                                          traceInfo.parameters.beginMarker,
                                          sizeof(payload.executeTraceRequestV5.parameters.beginMarker));

                        Platform::Strncpy(payload.executeTraceRequestV5.parameters.endMarker,
                                          traceInfo.parameters.endMarker,
                                          sizeof(payload.executeTraceRequestV5.parameters.endMarker));
                    }
                    // No further setup needed for CaptureTriggerMode::Present
                }
                else
                {
                    // Unhandled protocol version
                    DD_UNREACHABLE();
                }

                result = SendPayload(&payload);

                if (result == Result::Success)
                {
                    m_traceContext.traceInfo = traceInfo;
                    m_traceContext.state = TraceState::TraceRequested;
                }
                else
                {
                    // If we fail to send the payload, fail the trace.
                    m_traceContext.state = TraceState::Error;
                    result = Result::Error;
                }
            }

            return result;
        }

#if !DD_VERSION_SUPPORTS(GPUOPEN_LONG_RGP_TRACES_VERSION)
        Result RGPClient::EndTrace(uint32* pNumChunks, uint64* pTraceSizeInBytes)
        {
            Result result = Result::Error;

            if ((m_traceContext.state == TraceState::TraceRequested) &&
                (pNumChunks != nullptr)                              &&
                (pTraceSizeInBytes != nullptr))
            {
                if (GetSessionVersion() >= RGP_TRACE_PROGRESS_VERSION)
                {
                    RGPPayload payload = {};

                    const uint32 headerTimeout = kRGPChunkTimeoutInMs * (m_traceContext.traceInfo.parameters.numPreparationFrames + 1);

                    // Attempt to receive the trace data header.
                    result = ReceivePayload(&payload, headerTimeout);
                    if ((result == Result::Success) && (payload.command == RGPMessage::TraceDataHeader))
                    {
                        // We've successfully received the trace data header. Check if the trace was successful.
                        result = payload.traceDataHeader.result;
                        if (result == Result::Success)
                        {
                            m_traceContext.state = TraceState::TraceCompleted;
                            m_traceContext.numChunks = payload.traceDataHeader.numChunks;
                            m_traceContext.numChunksReceived = 0;

                            *pNumChunks = payload.traceDataHeader.numChunks;
                            *pTraceSizeInBytes = payload.traceDataHeader.sizeInBytes;
                        }
                        else
                        {
                            // Reset the trace state.
                            m_traceContext.state = TraceState::Error;

                            // Don't overwrite the result from the trace header here. We want to return that to the caller.
                        }
                    }
                    else
                    {
                        m_traceContext.state = TraceState::Error;
                        result = Result::Error;
                    }
                }
                else
                {
                    m_traceContext.state = TraceState::TraceCompleted;
                    result = Result::Unavailable;
                }
            }

            return result;
        }
#else
        Result RGPClient::EndTrace(uint32* pNumChunks, uint64* pTraceSizeInBytes, uint32 timeoutInMs)
        {
            Result result = Result::Error;

            if ((m_traceContext.state == TraceState::TraceRequested) &&
                (pNumChunks != nullptr)                              &&
                (pTraceSizeInBytes != nullptr))
            {
                if (GetSessionVersion() >= RGP_TRACE_PROGRESS_VERSION)
                {
                    RGPPayload payload = {};

                    // Attempt to receive the trace data header.
                    result = ReceivePayload(&payload, timeoutInMs);
                    if ((result == Result::Success) && (payload.command == RGPMessage::TraceDataHeader))
                    {
                        // We've successfully received the trace data header. Check if the trace was successful.
                        result = payload.traceDataHeader.result;
                        if (result == Result::Success)
                        {
                            m_traceContext.state = TraceState::TraceCompleted;
                            m_traceContext.numChunks = payload.traceDataHeader.numChunks;
                            m_traceContext.numChunksReceived = 0;

                            *pNumChunks = payload.traceDataHeader.numChunks;
                            *pTraceSizeInBytes = payload.traceDataHeader.sizeInBytes;
                        }
                        else
                        {
                            // Reset the trace state.
                            m_traceContext.state = TraceState::Error;

                            // Don't overwrite the result from the trace header here. We want to return that to the caller.
                        }
                    }
                    else if (result == Result::NotReady)
                    {
                        // If we hit the user specified timeout, don't modify the trace state.
                        // Just return the result to the caller.
                    }
                    else
                    {
                        m_traceContext.state = TraceState::Error;
                        result = Result::Error;
                    }
                }
                else
                {
                    m_traceContext.state = TraceState::TraceCompleted;
                    result = Result::Unavailable;
                }
            }

            return result;
        }
#endif

        Result RGPClient::ReadTraceDataChunk()
        {
            Result result = Result::Error;

            RGPPayload payload = {};

            if (m_traceContext.state == TraceState::TraceCompleted)
            {
                if (GetSessionVersion() >= RGP_TRACE_PROGRESS_VERSION)
                {
                    result = ReceivePayload(&payload, kRGPChunkTimeoutInMs);

                    if (result == Result::Success)
                    {
                        if ((payload.command == RGPMessage::TraceDataChunk) && (m_traceContext.numChunksReceived < m_traceContext.numChunks))
                        {
                            // Call the chunk callback with the trace data.
                            m_traceContext.traceInfo.callbackInfo.chunkCallback(&payload.traceDataChunk.chunk, m_traceContext.traceInfo.callbackInfo.pUserdata);

                            ++m_traceContext.numChunksReceived;

                            // If we have no chunks left, the trace process was successfully completed.
                            if (m_traceContext.numChunksReceived == m_traceContext.numChunks)
                            {
                                // Make sure we read the sentinel value before returning. It should always mark the end of the trace data chunk stream.
                                result = ReceivePayload(&payload, kRGPChunkTimeoutInMs);

                                if ((result == Result::Success) && (payload.command == RGPMessage::TraceDataSentinel))
                                {
                                    result = Result::EndOfStream;
                                    m_traceContext.state = TraceState::Idle;
                                }
                                else
                                {
                                    // Failed to receive a trace data chunk. Fail the trace.
                                    m_traceContext.state = TraceState::Error;
                                    result = Result::Error;
                                }
                            }
                        }
                        else
                        {
                            // Failed to receive a trace data chunk. Fail the trace.
                            m_traceContext.state = TraceState::Error;
                            result = Result::Error;
                        }
                    }
                    else
                    {
                        // Failed to receive a trace data chunk. Fail the trace.
                        m_traceContext.state = TraceState::Error;
                        result = Result::Error;
                    }
                }
                else if (GetSessionVersion() < RGP_TRACE_PROGRESS_VERSION)
                {
                    const uint32 firstChunkTimeout = kRGPChunkTimeoutInMs * (m_traceContext.traceInfo.parameters.numPreparationFrames + 1);
                    const uint32 packetTimeout = (m_traceContext.numChunksReceived == 0) ? firstChunkTimeout : kRGPChunkTimeoutInMs;

                    result = ReceivePayload(&payload, packetTimeout);

                    if (result == Result::Success)
                    {
                        if (payload.command == RGPMessage::TraceDataChunk)
                        {
                            // Call the chunk callback with the trace data.
                            m_traceContext.traceInfo.callbackInfo.chunkCallback(&payload.traceDataChunk.chunk, m_traceContext.traceInfo.callbackInfo.pUserdata);

                            ++m_traceContext.numChunksReceived;
                        }
                        else if (payload.command == RGPMessage::TraceDataSentinel)
                        {
                            result = Result::EndOfStream;
                        }
                    }
                    else
                    {
                        // Failed to receive a trace data chunk. Fail the trace.
                        m_traceContext.state = TraceState::Error;
                        result = Result::Error;
                    }
                }
            }

            return result;
        }

        Result RGPClient::AbortTrace()
        {
            Result result = Result::Error;

            RGPPayload payload = {};

            if ((m_traceContext.state == TraceState::TraceCompleted) |
                ((m_traceContext.state == TraceState::TraceRequested) & (GetSessionVersion() >= RGP_PENDING_ABORT_VERSION)))
            {
                if (GetSessionVersion() >= RGP_TRACE_PROGRESS_VERSION)
                {
                    payload.command = RGPMessage::AbortTrace;

                    result = SendPayload(&payload);

                    if (result == Result::Success)
                    {
                        // Discard all messages until we find the trace data sentinel.
                        while ((result == Result::Success) && (payload.command != RGPMessage::TraceDataSentinel))
                        {
                            result = ReceivePayload(&payload);
                        }

                        if ((result == Result::Success)                        &&
                            (payload.command == RGPMessage::TraceDataSentinel) &&
                            (payload.traceDataSentinel.result == Result::Aborted))
                        {
                            // We've successfully aborted the trace.
                            m_traceContext.state = TraceState::Idle;
                        }
                        else
                        {
                            // Fail the trace if this process does not succeed.
                            m_traceContext.state = TraceState::Error;
                            result = Result::Error;
                        }
                    }
                    else
                    {
                        // If we fail to send the payload, fail the trace.
                        m_traceContext.state = TraceState::Error;
                        result = Result::Error;
                    }
                }
                else
                {
                    // Support for aborting traces is not available until the trace progress version.
                    result = Result::Unavailable;
                }
            }

            return result;
        }

        Result RGPClient::QueryProfilingStatus(ProfilingStatus* pStatus)
        {
            Result result = Result::Error;

            if ((pStatus != nullptr) && IsConnected())
            {
                RGPPayload payload = {};
                payload.command = RGPMessage::QueryProfilingStatusRequest;

                if ((Transact(&payload) == Result::Success) &&
                    (payload.command == RGPMessage::QueryProfilingStatusResponse))
                {
                    *pStatus = payload.queryProfilingStatusResponse.status;
                    result = Result::Success;
                }
            }

            return result;
        }

        Result RGPClient::EnableProfiling()
        {
            Result result = Result::Error;

            if (IsConnected())
            {
                RGPPayload payload = {};
                payload.command = RGPMessage::EnableProfilingRequest;

                if ((Transact(&payload) == Result::Success) &&
                    (payload.command == RGPMessage::EnableProfilingResponse))
                {
                    result = payload.enableProfilingStatusResponse.result;
                }
            }

            return result;
        }

        void RGPClient::ResetState()
        {
            memset(&m_traceContext, 0, sizeof(m_traceContext));
        }
    }

} // DevDriver
