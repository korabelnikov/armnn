//
// Copyright © 2019 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include "SendCounterPacketTests.hpp"

#include <BufferManager.hpp>
#include <ProfilingUtils.hpp>
#include <SendTimelinePacket.hpp>
#include <TimelinePacketWriterFactory.hpp>

#include <boost/test/unit_test.hpp>

using namespace armnn::profiling;

BOOST_AUTO_TEST_SUITE(SendTimelinePacketTests)

BOOST_AUTO_TEST_CASE(SendTimelineMessageDirectoryPackageTest)
{
    MockBufferManager mockBuffer(512);
    TimelinePacketWriterFactory timelinePacketWriterFactory(mockBuffer);
    std::unique_ptr<ISendTimelinePacket> sendTimelinePacket = timelinePacketWriterFactory.GetSendTimelinePacket();

    sendTimelinePacket->SendTimelineMessageDirectoryPackage();

    // Get the readable buffer
    auto packetBuffer = mockBuffer.GetReadableBuffer();

    unsigned int uint32_t_size = sizeof(uint32_t);
    // Check the packet header
    unsigned int offset = 0;
    uint32_t packetHeaderWord0 = ReadUint32(packetBuffer, offset);
    uint32_t packetFamily = (packetHeaderWord0 >> 26) & 0x0000003F;
    uint32_t packetClass  = (packetHeaderWord0 >> 19) & 0x0000007F;
    uint32_t packetType   = (packetHeaderWord0 >> 16) & 0x00000007;
    uint32_t streamId     = (packetHeaderWord0 >>  0) & 0x00000007;

    BOOST_CHECK(packetFamily == 1);
    BOOST_CHECK(packetClass  == 0);
    BOOST_CHECK(packetType   == 0);
    BOOST_CHECK(streamId     == 0);

    offset += uint32_t_size;
    uint32_t packetHeaderWord1 = ReadUint32(packetBuffer, offset);
    uint32_t sequenceNumbered = (packetHeaderWord1 >> 24) & 0x00000001;
    uint32_t dataLength       = (packetHeaderWord1 >>  0) & 0x00FFFFFF;
    BOOST_CHECK(sequenceNumbered ==  0);
    BOOST_CHECK(dataLength       == 416);

    offset += uint32_t_size;
    SwTraceMessage swTraceMessage = ReadSwTraceMessage(packetBuffer, offset);

    BOOST_CHECK(swTraceMessage.id == 0);
    BOOST_CHECK(swTraceMessage.name == "declareLabel");
    BOOST_CHECK(swTraceMessage.uiName == "declare label");
    BOOST_CHECK(swTraceMessage.argTypes.size() == 2);
    BOOST_CHECK(swTraceMessage.argTypes[0] == 'p');
    BOOST_CHECK(swTraceMessage.argTypes[1] == 's');
    BOOST_CHECK(swTraceMessage.argNames.size() == 2);
    BOOST_CHECK(swTraceMessage.argNames[0] == "guid");
    BOOST_CHECK(swTraceMessage.argNames[1] == "value");

    swTraceMessage = ReadSwTraceMessage(packetBuffer, offset);

    BOOST_CHECK(swTraceMessage.id == 1);
    BOOST_CHECK(swTraceMessage.name == "declareEntity");
    BOOST_CHECK(swTraceMessage.uiName == "declare entity");
    BOOST_CHECK(swTraceMessage.argTypes.size() == 1);
    BOOST_CHECK(swTraceMessage.argTypes[0] == 'p');
    BOOST_CHECK(swTraceMessage.argNames.size() == 1);
    BOOST_CHECK(swTraceMessage.argNames[0] == "guid");

    swTraceMessage = ReadSwTraceMessage(packetBuffer, offset);

    BOOST_CHECK(swTraceMessage.id == 2);
    BOOST_CHECK(swTraceMessage.name == "declareEventClass");
    BOOST_CHECK(swTraceMessage.uiName == "declare event class");
    BOOST_CHECK(swTraceMessage.argTypes.size() == 1);
    BOOST_CHECK(swTraceMessage.argTypes[0] == 'p');
    BOOST_CHECK(swTraceMessage.argNames.size() == 1);
    BOOST_CHECK(swTraceMessage.argNames[0] == "guid");

    swTraceMessage = ReadSwTraceMessage(packetBuffer, offset);

    BOOST_CHECK(swTraceMessage.id == 3);
    BOOST_CHECK(swTraceMessage.name == "declareRelationship");
    BOOST_CHECK(swTraceMessage.uiName == "declare relationship");
    BOOST_CHECK(swTraceMessage.argTypes.size() == 4);
    BOOST_CHECK(swTraceMessage.argTypes[0] == 'I');
    BOOST_CHECK(swTraceMessage.argTypes[1] == 'p');
    BOOST_CHECK(swTraceMessage.argTypes[2] == 'p');
    BOOST_CHECK(swTraceMessage.argTypes[3] == 'p');
    BOOST_CHECK(swTraceMessage.argNames.size() == 4);
    BOOST_CHECK(swTraceMessage.argNames[0] == "relationshipType");
    BOOST_CHECK(swTraceMessage.argNames[1] == "relationshipGuid");
    BOOST_CHECK(swTraceMessage.argNames[2] == "headGuid");
    BOOST_CHECK(swTraceMessage.argNames[3] == "tailGuid");

    swTraceMessage = ReadSwTraceMessage(packetBuffer, offset);

    BOOST_CHECK(swTraceMessage.id == 4);
    BOOST_CHECK(swTraceMessage.name == "declareEvent");
    BOOST_CHECK(swTraceMessage.uiName == "declare event");
    BOOST_CHECK(swTraceMessage.argTypes.size() == 3);
    BOOST_CHECK(swTraceMessage.argTypes[0] == '@');
    BOOST_CHECK(swTraceMessage.argTypes[1] == 't');
    BOOST_CHECK(swTraceMessage.argTypes[2] == 'p');
    BOOST_CHECK(swTraceMessage.argNames.size() == 3);
    BOOST_CHECK(swTraceMessage.argNames[0] == "timestamp");
    BOOST_CHECK(swTraceMessage.argNames[1] == "threadId");
    BOOST_CHECK(swTraceMessage.argNames[2] == "eventGuid");
}

BOOST_AUTO_TEST_CASE(SendTimelineEntityPlusEventClassBinaryPacketTest)
{
    MockBufferManager bufferManager(40);
    TimelinePacketWriterFactory timelinePacketWriterFactory(bufferManager);
    std::unique_ptr<ISendTimelinePacket> sendTimelinePacket = timelinePacketWriterFactory.GetSendTimelinePacket();

    const uint64_t entityBinaryPacketProfilingGuid = 123456u;
    sendTimelinePacket->SendTimelineEntityBinaryPacket(entityBinaryPacketProfilingGuid);

    const uint64_t eventClassBinaryPacketProfilingGuid = 789123u;
    sendTimelinePacket->SendTimelineEventClassBinaryPacket(eventClassBinaryPacketProfilingGuid);

    // Commit the messages
    sendTimelinePacket->Commit();

    // Get the readable buffer
    auto packetBuffer = bufferManager.GetReadableBuffer();

    unsigned int uint32_t_size = sizeof(uint32_t);
    unsigned int uint64_t_size = sizeof(uint64_t);

    // Check the packet header
    unsigned int offset = 0;

    // Reading TimelineEntityClassBinaryPacket
    uint32_t entityBinaryPacketHeaderWord0 = ReadUint32(packetBuffer, offset);
    uint32_t entityBinaryPacketFamily = (entityBinaryPacketHeaderWord0 >> 26) & 0x0000003F;
    uint32_t entityBinaryPacketClass  = (entityBinaryPacketHeaderWord0 >> 19) & 0x0000007F;
    uint32_t entityBinaryPacketType   = (entityBinaryPacketHeaderWord0 >> 16) & 0x00000007;
    uint32_t entityBinaryPacketStreamId     = (entityBinaryPacketHeaderWord0 >>  0) & 0x00000007;

    BOOST_CHECK(entityBinaryPacketFamily == 1);
    BOOST_CHECK(entityBinaryPacketClass  == 0);
    BOOST_CHECK(entityBinaryPacketType   == 1);
    BOOST_CHECK(entityBinaryPacketStreamId     == 0);

    offset += uint32_t_size;
    uint32_t entityBinaryPacketHeaderWord1 = ReadUint32(packetBuffer, offset);
    uint32_t entityBinaryPacketSequenceNumbered = (entityBinaryPacketHeaderWord1 >> 24) & 0x00000001;
    uint32_t entityBinaryPacketDataLength       = (entityBinaryPacketHeaderWord1 >>  0) & 0x00FFFFFF;
    BOOST_CHECK(entityBinaryPacketSequenceNumbered == 0);
    BOOST_CHECK(entityBinaryPacketDataLength       == 8);

    // Check the decl_id
    offset += uint32_t_size;
    uint32_t entitytDecId = ReadUint32(packetBuffer, offset);

    BOOST_CHECK(entitytDecId == uint32_t(1));

    // Check the profiling GUID
    offset += uint32_t_size;
    uint64_t readProfilingGuid = ReadUint64(packetBuffer, offset);

    BOOST_CHECK(readProfilingGuid == entityBinaryPacketProfilingGuid);

    // Reading TimelineEventClassBinaryPacket
    offset += uint64_t_size;
    uint32_t eventClassBinaryPacketHeaderWord0 = ReadUint32(packetBuffer, offset);
    uint32_t eventClassBinaryPacketFamily = (eventClassBinaryPacketHeaderWord0 >> 26) & 0x0000003F;
    uint32_t eventClassBinaryPacketClass  = (eventClassBinaryPacketHeaderWord0 >> 19) & 0x0000007F;
    uint32_t eventClassBinaryPacketType   = (eventClassBinaryPacketHeaderWord0 >> 16) & 0x00000007;
    uint32_t eventClassBinaryPacketStreamId     = (eventClassBinaryPacketHeaderWord0 >>  0) & 0x00000007;

    BOOST_CHECK(eventClassBinaryPacketFamily == 1);
    BOOST_CHECK(eventClassBinaryPacketClass  == 0);
    BOOST_CHECK(eventClassBinaryPacketType   == 1);
    BOOST_CHECK(eventClassBinaryPacketStreamId     == 0);

    offset += uint32_t_size;
    uint32_t eventClassBinaryPacketHeaderWord1 = ReadUint32(packetBuffer, offset);
    uint32_t eventClassBinaryPacketSequenceNumbered = (eventClassBinaryPacketHeaderWord1 >> 24) & 0x00000001;
    uint32_t eventClassBinaryPacketDataLength       = (eventClassBinaryPacketHeaderWord1 >>  0) & 0x00FFFFFF;
    BOOST_CHECK(eventClassBinaryPacketSequenceNumbered == 0);
    BOOST_CHECK(eventClassBinaryPacketDataLength       == 12);

    offset += uint32_t_size;
    uint32_t eventClassDeclId = ReadUint32(packetBuffer, offset);
    BOOST_CHECK(eventClassDeclId == uint32_t(2));

    // Check the profiling GUID
    offset += uint32_t_size;
    readProfilingGuid = ReadUint64(packetBuffer, offset);
    BOOST_CHECK(readProfilingGuid == eventClassBinaryPacketProfilingGuid);

    bufferManager.MarkRead(packetBuffer);
}

BOOST_AUTO_TEST_CASE(SendTimelinePacketTests1)
{
    unsigned int uint32_t_size = sizeof(uint32_t);
    unsigned int uint64_t_size = sizeof(uint64_t);

    MockBufferManager bufferManager(512);
    TimelinePacketWriterFactory timelinePacketWriterFactory(bufferManager);
    std::unique_ptr<ISendTimelinePacket> sendTimelinePacket = timelinePacketWriterFactory.GetSendTimelinePacket();

    // Send TimelineEntityClassBinaryPacket
    const uint64_t entityBinaryPacketProfilingGuid = 123456u;
    sendTimelinePacket->SendTimelineEntityBinaryPacket(entityBinaryPacketProfilingGuid);

    // Commit the buffer
    sendTimelinePacket->Commit();

    // Get the readable buffer
    auto packetBuffer = bufferManager.GetReadableBuffer();

    // Check the packet header
    unsigned int offset = 0;

    // Reading TimelineEntityClassBinaryPacket
    uint32_t entityBinaryPacketHeaderWord0 = ReadUint32(packetBuffer, offset);
    uint32_t entityBinaryPacketFamily = (entityBinaryPacketHeaderWord0 >> 26) & 0x0000003F;
    uint32_t entityBinaryPacketClass  = (entityBinaryPacketHeaderWord0 >> 19) & 0x0000007F;
    uint32_t entityBinaryPacketType   = (entityBinaryPacketHeaderWord0 >> 16) & 0x00000007;
    uint32_t entityBinaryPacketStreamId     = (entityBinaryPacketHeaderWord0 >>  0) & 0x00000007;

    BOOST_CHECK(entityBinaryPacketFamily == 1);
    BOOST_CHECK(entityBinaryPacketClass  == 0);
    BOOST_CHECK(entityBinaryPacketType   == 1);
    BOOST_CHECK(entityBinaryPacketStreamId     == 0);

    offset += uint32_t_size;
    uint32_t entityBinaryPacketHeaderWord1 = ReadUint32(packetBuffer, offset);
    uint32_t entityBinaryPacketSequenceNumbered = (entityBinaryPacketHeaderWord1 >> 24) & 0x00000001;
    uint32_t entityBinaryPacketDataLength       = (entityBinaryPacketHeaderWord1 >>  0) & 0x00FFFFFF;
    BOOST_CHECK(entityBinaryPacketSequenceNumbered == 0);
    BOOST_CHECK(entityBinaryPacketDataLength       == 8);

    // Check the decl_id
    offset += uint32_t_size;
    uint32_t entitytDecId = ReadUint32(packetBuffer, offset);

    BOOST_CHECK(entitytDecId == uint32_t(1));

    // Check the profiling GUID
    offset += uint32_t_size;
    uint64_t readProfilingGuid = ReadUint64(packetBuffer, offset);

    BOOST_CHECK(readProfilingGuid == entityBinaryPacketProfilingGuid);

    bufferManager.MarkRead(packetBuffer);

    // Send TimelineEventClassBinaryPacket
    const uint64_t eventClassBinaryPacketProfilingGuid = 789123u;
    sendTimelinePacket->SendTimelineEventClassBinaryPacket(eventClassBinaryPacketProfilingGuid);

    // Commit the buffer
    sendTimelinePacket->Commit();

    // Get the readable buffer
    packetBuffer = bufferManager.GetReadableBuffer();

    // Check the packet header
    offset = 0;

    // Reading TimelineEventClassBinaryPacket
    uint32_t eventClassBinaryPacketHeaderWord0 = ReadUint32(packetBuffer, offset);
    uint32_t eventClassBinaryPacketFamily = (eventClassBinaryPacketHeaderWord0 >> 26) & 0x0000003F;
    uint32_t eventClassBinaryPacketClass  = (eventClassBinaryPacketHeaderWord0 >> 19) & 0x0000007F;
    uint32_t eventClassBinaryPacketType   = (eventClassBinaryPacketHeaderWord0 >> 16) & 0x00000007;
    uint32_t eventClassBinaryPacketStreamId     = (eventClassBinaryPacketHeaderWord0 >>  0) & 0x00000007;

    BOOST_CHECK(eventClassBinaryPacketFamily == 1);
    BOOST_CHECK(eventClassBinaryPacketClass  == 0);
    BOOST_CHECK(eventClassBinaryPacketType   == 1);
    BOOST_CHECK(eventClassBinaryPacketStreamId     == 0);

    offset += uint32_t_size;
    uint32_t eventClassBinaryPacketHeaderWord1 = ReadUint32(packetBuffer, offset);
    uint32_t eventClassBinaryPacketSequenceNumbered = (eventClassBinaryPacketHeaderWord1 >> 24) & 0x00000001;
    uint32_t eventClassBinaryPacketDataLength       = (eventClassBinaryPacketHeaderWord1 >>  0) & 0x00FFFFFF;
    BOOST_CHECK(eventClassBinaryPacketSequenceNumbered == 0);
    BOOST_CHECK(eventClassBinaryPacketDataLength       == 12);

    offset += uint32_t_size;
    uint32_t eventClassDeclId = ReadUint32(packetBuffer, offset);
    BOOST_CHECK(eventClassDeclId == uint32_t(2));

    // Check the profiling GUID
    offset += uint32_t_size;
    readProfilingGuid = ReadUint64(packetBuffer, offset);
    BOOST_CHECK(readProfilingGuid == eventClassBinaryPacketProfilingGuid);

    bufferManager.MarkRead(packetBuffer);

    // Send TimelineEventBinaryPacket
    const uint64_t timestamp = 456789u;
    const uint32_t threadId = 654321u;
    const uint64_t eventProfilingGuid = 123456u;
    sendTimelinePacket->SendTimelineEventBinaryPacket(timestamp, threadId, eventProfilingGuid);

    // Commit the buffer
    sendTimelinePacket->Commit();

    // Get the readable buffer
    packetBuffer = bufferManager.GetReadableBuffer();

    // Check the packet header
    offset = 0;

    // Reading TimelineEventBinaryPacket
    uint32_t eventBinaryPacketHeaderWord0 = ReadUint32(packetBuffer, offset);
    uint32_t eventBinaryPacketFamily = (eventBinaryPacketHeaderWord0 >> 26) & 0x0000003F;
    uint32_t eventBinaryPacketClass  = (eventBinaryPacketHeaderWord0 >> 19) & 0x0000007F;
    uint32_t eventBinaryPacketType   = (eventBinaryPacketHeaderWord0 >> 16) & 0x00000007;
    uint32_t eventBinaryPacketStreamId     = (eventBinaryPacketHeaderWord0 >>  0) & 0x00000007;

    BOOST_CHECK(eventBinaryPacketFamily == 1);
    BOOST_CHECK(eventBinaryPacketClass  == 0);
    BOOST_CHECK(eventBinaryPacketType   == 1);
    BOOST_CHECK(eventBinaryPacketStreamId     == 0);

    offset += uint32_t_size;
    uint32_t eventBinaryPacketHeaderWord1 = ReadUint32(packetBuffer, offset);
    uint32_t eventBinaryPacketSequenceNumbered = (eventBinaryPacketHeaderWord1 >> 24) & 0x00000001;
    uint32_t eventBinaryPacketDataLength       = (eventBinaryPacketHeaderWord1 >>  0) & 0x00FFFFFF;
    BOOST_CHECK(eventBinaryPacketSequenceNumbered == 0);
    BOOST_CHECK(eventBinaryPacketDataLength       == 24);

    // Check the decl_id
    offset += uint32_t_size;
    uint32_t eventDeclId = ReadUint32(packetBuffer, offset);
    BOOST_CHECK(eventDeclId == 4);

    // Check the timestamp
    offset += uint32_t_size;
    uint64_t eventTimestamp = ReadUint64(packetBuffer, offset);
    BOOST_CHECK(eventTimestamp == timestamp);

    // Check the thread id
    offset += uint64_t_size;
    uint32_t readThreadId = ReadUint32(packetBuffer, offset);
    BOOST_CHECK(readThreadId == threadId);

    // Check the profiling GUID
    offset += uint32_t_size;
    readProfilingGuid = ReadUint64(packetBuffer, offset);
    BOOST_CHECK(readProfilingGuid == eventProfilingGuid);
}

BOOST_AUTO_TEST_CASE(SendTimelinePacketTests2)
{
    MockBufferManager bufferManager(40);
    TimelinePacketWriterFactory timelinePacketWriterFactory(bufferManager);
    std::unique_ptr<ISendTimelinePacket> sendTimelinePacket = timelinePacketWriterFactory.GetSendTimelinePacket();

    BOOST_CHECK_THROW(sendTimelinePacket->SendTimelineMessageDirectoryPackage(),
                      armnn::RuntimeException);
}

BOOST_AUTO_TEST_CASE(SendTimelinePacketTests3)
{
    MockBufferManager bufferManager(512);
    TimelinePacketWriterFactory timelinePacketWriterFactory(bufferManager);
    std::unique_ptr<ISendTimelinePacket> sendTimelinePacket = timelinePacketWriterFactory.GetSendTimelinePacket();

    // Send TimelineEntityClassBinaryPacket
    const uint64_t entityBinaryPacketProfilingGuid = 123456u;
    sendTimelinePacket->SendTimelineEntityBinaryPacket(entityBinaryPacketProfilingGuid);

    // Commit the buffer
    sendTimelinePacket->Commit();

    // Get the readable buffer
    auto packetBuffer = bufferManager.GetReadableBuffer();

    // Send TimelineEventClassBinaryPacket
    const uint64_t eventClassBinaryPacketProfilingGuid = 789123u;
    BOOST_CHECK_THROW(sendTimelinePacket->SendTimelineEventClassBinaryPacket(eventClassBinaryPacketProfilingGuid),
                      armnn::RuntimeException);
}

BOOST_AUTO_TEST_SUITE_END()