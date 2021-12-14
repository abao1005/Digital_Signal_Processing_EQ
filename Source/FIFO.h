/*
  ==============================================================================
    FIFO.h
    Created: 4 Oct 2021 4:37:37pm
    Author:  tuiji da
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

template<typename T>
class FIFO
{
public:
    void prepare(int numChannels, int numSamples)
    {
        for( auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                           numSamples,
                           false,   //clear everything?
                           true,    //including the extra space?
                           true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }
    
    void prepare(int numElements)
    {
        for( auto& buffer : buffers )
        {
            buffer.clear();
            buffer.resize(1, numElements);
        }
    }
    
    bool push(const T& b)
    {
        auto write = fifo.write(1);
        if( write.blockSize1 > 0 )
        {
            buffers[write.startIndex1] = b;
            return true;
        }
        
        return false;
    }
    
    bool pop(T& b)
    {
        auto read = fifo.read(1);
        if( read.blockSize1 > 0 )
        {
            b = buffers[read.startIndex1];
            return true;
        }
        
        return false;
    }
    
    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
    
private:
    static const int Capacity {10};
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo {Capacity};
};

template<typename BlockType>
class SingleChannelSampleFifo
{
public:
    SingleChannelSampleFifo(int ch) : channelToUse(ch)
    {
        prepared.set(false);
    }
    
    void update(const BlockType& buffer)
    {
        
        auto* channelPtr = buffer.getReadPointer(channelToUse);
        
        for( int i = 0; i < buffer.getNumSamples(); ++i )
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);
        
        bufferToFill.setSize(1,             //channel
                             bufferSize,    //num samples
                             false,         //keepExistingContent
                             true,          //clear extra space
                             true);         //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pop(buf); }

private:
    int channelToUse;
    int fifoIndex = 0;
    FIFO<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;
    
    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);
            
            fifoIndex = 0;
        }
        
        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

template<typename PathType>
class PathProducer
{
public:
    
    void generatePath(juce::AudioBuffer<float>& data, juce::Rectangle<float> bounds)
    {
        auto top = bounds.getY();
        auto bottom = bounds.getHeight();
        
        auto map = [bottom, top](float v)
        {
            return juce::jmap(v, -1.f, 1.f, bottom, top);
        };

        PathType p;
        
        auto y = map(data.getSample(0, 0));
        
        if( std::isnan(y) || std::isinf(y) )
            y = bottom;
        
        p.startNewSubPath(0, y);
        
        for (int i = 1; i < data.getNumSamples(); i++)
        {
            auto y = map(data.getSample(0, i));
            
            if(!std::isnan(y) && !std::isinf(y))
            {
                p.lineTo(i, y);
            }
        }
        
        pathFifo.push(p);
    }
    
    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pop(path);
    }

private:
    FIFO<PathType> pathFifo;
};

template<typename BlockType>
class FFTDataGenerator
{
public:
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();
                
        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());
        
        // first apply a windowing function to our data
        window->multiplyWithWindowingTable (fftData.data(), fftSize);       // [1]
        
        // then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform (fftData.data());  // [2]
        
        int numBins = (int)fftSize / 2;
        
        //normalize the fft values.
        for( int i = 0; i < numBins; ++i )
        {
            auto v = fftData[i];
//            fftData[i] /= (float) numBins;
            if( !std::isinf(v) && !std::isnan(v) )
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = v;
        }
        
        //convert them to decibels
        for( int i = 0; i < numBins; ++i )
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }
        
        fftDataFifo.push(fftData);
    }
    
    void changeOrder(int newOrder)
    {
        order = newOrder;
        auto fftSize = getFFTSize();
        
        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);
        
        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    
    int getFFTSize() const { return 1 << order; }
    
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pop(fftData); }
    
private:
    int order = 10;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    FIFO<BlockType> fftDataFifo;
};

template<typename PathType>
class FFTPathProducer
{
public:
    
    void generatePath(const std::vector<float>& renderData,
                      juce::Rectangle<float> fftBounds,
                      int fftSize,
                      float binWidth,
                      float negativeInfinity)
    {
        auto left = fftBounds.getX();
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                              negativeInfinity, 0.f,
                              float(bottom+10),   top);
        };

        auto y = map(renderData[0]);
        if( std::isnan(y) || std::isinf(y) )
            y = bottom;
        
        // p.startNewSubPath(0, bottom+10);
        // p.lineTo(left, bottom+10);
        // p.closeSubPath();
        // p.startNewSubPath(left + width, bottom+10);
        // p.lineTo(2 * width, bottom+10);
        // p.closeSubPath();
        // if (y != bottom + 10)
        //     p.startNewSubPath(left, bottom+10);
        // else
        //     p.startNewSubPath(left, y);
        // p.lineTo(left, y);
        
        p.startNewSubPath(left, y);

        const int pathResolution = 2;
        for( int binNum = 1; binNum < numBins; binNum += pathResolution )
        {
            y = map(renderData[binNum]);

            if( !std::isnan(y) && !std::isinf(y) )
            {
                auto binFreq = binNum * binWidth;
                // auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                auto normalizedBinX = (binFreq - 20.f) / (20000.f - 20.f);
                int binX = std::floor(left + normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
        
    }
    
    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pop(path);
    }

private:
    FIFO<PathType> pathFifo;
};