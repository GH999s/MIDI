#pragma once

BEGIN_MIDI_NAMESPACE

template<unsigned BufferSize>
inline UsbTransport<BufferSize>::UsbTransport()
{

}

template<unsigned BufferSize>
inline UsbTransport<BufferSize>::~UsbTransport()
{

}

// -----------------------------------------------------------------------------

template<unsigned BufferSize>
inline void UsbTransport<BufferSize>::begin(unsigned inBaudrate)
{
    mTxBuffer.clear();
    mRxBuffer.clear();
}

template<unsigned BufferSize>
inline unsigned UsbTransport<BufferSize>::available()
{
    pollUsbMidi();
    return mRxBuffer.getLength();
}

template<unsigned BufferSize>
inline byte UsbTransport<BufferSize>::read()
{
    return mRxBuffer.read();
}

template<unsigned BufferSize>
inline void UsbTransport<BufferSize>::write(byte inData)
{
    mTxBuffer.write(inData);
    recomposeAndSendTxPackets();
}

// -----------------------------------------------------------------------------

template<unsigned BufferSize>
inline bool UsbTransport<BufferSize>::pollUsbMidi()
{
    bool received = false;
    midiEventPacket_t packet = MidiUSB.read();
    while (packet.header != 0)
    {
        received = true;

        switch (packet.header << 4)
        {
                // 3 bytes messages
            case midi::NoteOff:
            case midi::NoteOn:
            case midi::AfterTouchPoly:
            case midi::ControlChange:
            case midi::PitchBend:
                mRxBuffer.write(packet.byte1);
                mRxBuffer.write(packet.byte2);
                mRxBuffer.write(packet.byte3);
                break;

                // 2 bytes messages
            case midi::ProgramChange:
            case midi::AfterTouchChannel:
                mRxBuffer.write(packet.byte1);
                mRxBuffer.write(packet.byte2);
                break;

                // 1 byte message
            case midi::TuneRequest:
            case midi::Clock:
            case midi::Start:
            case midi::Continue:
            case midi::Stop:
            case midi::ActiveSensing:
            case midi::SystemReset:
                mRxBuffer.write(packet.byte1);
                break;

                // Special cases
                // case midi::SystemExclusive:
                // case midi::TimeCodeQuarterFrame:
                // case midi::SongPosition:
                // case midi::SongSelect:
                //   break;

            default:
                break;
        }

        packet = MidiUSB.read();
    }
    return received;
}

template<unsigned BufferSize>
inline void UsbTransport<BufferSize>::recomposeAndSendTxPackets()
{
    while (!mTxBuffer.isEmpty())
    {
        const byte data = mTxBuffer.read();
        if (mCurrentTxPacketByteIndex == 0)
        {
            mCurrentTxPacket.mPacket.header = encodePacketHeader(data);
        }
        else
        {
            mCurrentTxPacket.mDataArray[mCurrentTxPacketByteIndex] = data;
        }
        mCurrentTxPacketByteIndex++;

        const int packetLength = getPacketLength(mCurrentTxPacket.mPacket);

        if (mCurrentTxPacketByteIndex == packetLength)
        {
            MidiUSB.write(mCurrentTxPacket.mDataArray, packetLength);
            resetTx();
        }
    }
}

template<unsigned BufferSize>
inline void UsbTransport<BufferSize>::resetTx()
{
    mCurrentTxPacket.mPacket.header = 0;
    mCurrentTxPacket.mPacket.byte1  = 0;
    mCurrentTxPacket.mPacket.byte2  = 0;
    mCurrentTxPacket.mPacket.byte3  = 0;
    mCurrentTxPacketByteIndex = 0;
}

template<unsigned BufferSize>
inline byte UsbTransport<BufferSize>::encodePacketHeader(StatusByte inStatusByte)
{
    // todo: fix me for other types than channel
    return inStatusByte >> 4;
}

template<unsigned BufferSize>
inline int UsbTransport<BufferSize>::getPacketLength(const midiEventPacket_t& inPacket)
{
    return 3;
}

END_MIDI_NAMESPACE
