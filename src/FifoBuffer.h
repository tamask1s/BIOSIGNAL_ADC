/*
This software is licensed under MIT License.

Copyright (c) 2019 Tamas Levente Kis - tamkis@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef CFIFOBUFFER_HH
#define CFIFOBUFFER_HH

template<typename TType>
class fifo_buffer
{
    TType* mData;
    unsigned int mRealSize;
    unsigned int mShift;
protected:
    unsigned int mSize;
public:
    TType* mShiftedData;
    fifo_buffer(size_t a_size)
    {
        mData = new TType[a_size];
        mShiftedData = mData;
        mRealSize = a_size;
        mSize = 0;
        mShift = 0;
    }
    ~fifo_buffer()
    {
        delete[] mData;
    }
    bool empty() const
    {
        return mSize ? 0 : 1;
    }
    size_t size() const
    {
        return mSize;
    }
    void push_back(const TType& aElement)
    {
        push_elements_back(&aElement, 1);
    }
    void push_elements_back(const TType* aElements, size_t aNrElements)
    {
        if (mShift + mSize + aNrElements > mRealSize)
        {
            if ((mSize + aNrElements <= mRealSize) && (mShift > (mRealSize >> 1)) && ((mRealSize >> 1) >= aNrElements))
                memcpy(mData, mShiftedData, mSize * sizeof(TType));
            else
            {
                mRealSize <<= 1;
                if ((mRealSize - mSize) < aNrElements)
                    mRealSize += aNrElements;
                TType* lNewData = new TType[mRealSize];
                memcpy(lNewData, mShiftedData, mSize * sizeof(TType));
                delete[] mData;
                mData = lNewData;
            }
            mShift = 0;
            mShiftedData = mData;
        }
        memcpy(mShiftedData + mSize, aElements, aNrElements * sizeof(TType));
        mSize += aNrElements;
    }
    TType* enlarge_back(size_t aNrElements)
    {
        if (mShift + mSize + aNrElements > mRealSize)
        {
            if ((mSize + aNrElements <= mRealSize) && (mShift > (mRealSize >> 1)) && ((mRealSize >> 1) >= aNrElements))
                memcpy(mData, mShiftedData, mSize * sizeof(TType));
            else
            {
                mRealSize <<= 1;
                if ((mRealSize - mSize) < aNrElements)
                    mRealSize += aNrElements;
                TType* lNewData = new TType[mRealSize];
                memcpy(lNewData, mShiftedData, mSize * sizeof(TType));
                delete[] mData;
                mData = lNewData;
            }
            mShift = 0;
            mShiftedData = mData;
        }
        mSize += aNrElements;
        return mShiftedData + mSize - aNrElements;
    }
    void clear()
    {
        mShiftedData = mData;
        mSize = 0;
        mShift = 0;
    }
    TType& operator[] (size_t aIndex) const
    {
        return mShiftedData[aIndex];
    }
    TType& front() const
    {
        return mShiftedData[0];
    }
    TType* front_elements() const
    {
        return mShiftedData;
    }
    TType& back() const
    {
        return mShiftedData[mSize - 1];
    }
    void pop_front()
    {
        if (mSize)
        {
            ++mShiftedData;
            ++mShift;
            --mSize;
        }
    }
    void pop_back()
    {
        if (mSize)
            --mSize;
    }
    void pop_elements_front(size_t aNrElements)
    {
        if (mSize >= aNrElements)
        {
            mShiftedData += aNrElements;
            mShift += aNrElements;
            mSize -= aNrElements;
        }
    }
    void pop_elements_back(size_t aNrElements)
    {
        if (mSize >= aNrElements)
            mSize -= aNrElements;
    }
};

template<size_t nr_max_packets = 100>
class io_buffer
{
    int packet_bytes_;
    int nr_max_packets_;
    std::vector<uint8_t> buffer_;
    int it_read = 0;
    int it_write = 0;
    int it_write_last = 0;
    volatile uint8_t rw_states_[nr_max_packets];

public:
    io_buffer(int packet_size)
        : packet_bytes_(packet_size),
          nr_max_packets_(nr_max_packets)
    {
        buffer_.resize(packet_bytes_ * nr_max_packets_);
        for (int i = 0; i < nr_max_packets_; ++i)
            rw_states_[i] = 0;
    }

    uint8_t* get_next_filled_address()
    {
        uint8_t* res = 0;
        if (rw_states_[it_read] == 2)
        {
            res = &buffer_[it_read * packet_bytes_];
            rw_states_[it_read] = 3;
            ++it_read;
            if (it_read == nr_max_packets_)
                it_read = 0;
        }
        return res;
    }

    uint8_t* get_next_address_to_fill()
    {
        uint8_t* res = 0;
        if (rw_states_[it_write] == 0 || rw_states_[it_write] == 3)
        {
            if (rw_states_[it_write_last] == 1)
                rw_states_[it_write_last] = 2;
            res = &buffer_[it_write * packet_bytes_];
            rw_states_[it_write] = 1;
            it_write_last = it_write;
            ++it_write;
            if (it_write == nr_max_packets_)
                it_write = 0;
        }
        return res;
    }
};

#endif /// CFIFOBUFFER_HH
