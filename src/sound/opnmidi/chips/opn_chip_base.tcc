#include "opn_chip_base.h"
#include <cmath>

#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
#include <zita-resampler/vresampler.h>
#endif

#if !defined(LIKELY) && defined(__GNUC__)
#define LIKELY(x) __builtin_expect((x), 1)
#elif !defined(LIKELY)
#define LIKELY(x) (x)
#endif

#if !defined(UNLIKELY) && defined(__GNUC__)
#define UNLIKELY(x) __builtin_expect((x), 0)
#elif !defined(UNLIKELY)
#define UNLIKELY(x) (x)
#endif

/* OPNChipBase */

inline OPNChipBase::OPNChipBase() :
    m_id(0),
    m_rate(44100),
    m_clock(7670454)
{
}

inline OPNChipBase::~OPNChipBase()
{
}

/* OPNChipBaseT */

template <class T>
OPNChipBaseT<T>::OPNChipBaseT()
    : OPNChipBase(),
      m_runningAtPcmRate(false)
#if defined(OPNMIDI_AUDIO_TICK_HANDLER)
    ,
      m_audioTickHandlerInstance(NULL)
#endif
{
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    m_resampler = new VResampler;
#endif
    setupResampler(m_rate);
}

template <class T>
OPNChipBaseT<T>::~OPNChipBaseT()
{
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    delete m_resampler;
#endif
}

template <class T>
bool OPNChipBaseT<T>::isRunningAtPcmRate() const
{
    return m_runningAtPcmRate;
}

template <class T>
bool OPNChipBaseT<T>::setRunningAtPcmRate(bool r)
{
    if(r != m_runningAtPcmRate)
    {
        if(r && !static_cast<T *>(this)->canRunAtPcmRate())
            return false;
        m_runningAtPcmRate = r;
        static_cast<T *>(this)->setRate(m_rate, m_clock);
    }
    return true;
}

#if defined(OPNMIDI_AUDIO_TICK_HANDLER)
template <class T>
void OPNChipBaseT<T>::setAudioTickHandlerInstance(void *instance)
{
    m_audioTickHandlerInstance = instance;
}
#endif

template <class T>
void OPNChipBaseT<T>::setRate(uint32_t rate, uint32_t clock)
{
    uint32_t oldRate = m_rate;
    m_rate = rate;
    m_clock = clock;
    if(rate != oldRate)
        setupResampler(rate);
    else
        resetResampler();
}

template <class T>
uint32_t OPNChipBaseT<T>::effectiveRate() const
{
    return m_runningAtPcmRate ? m_rate : (uint32_t)nativeRate;
}

template <class T>
void OPNChipBaseT<T>::reset()
{
    resetResampler();
}

template <class T>
void OPNChipBaseT<T>::generate(int16_t *output, size_t frames)
{
    static_cast<T *>(this)->nativePreGenerate();
    for(size_t i = 0; i < frames; ++i)
    {
        int32_t frame[2];
        static_cast<T *>(this)->resampledGenerate(frame);
        for (unsigned c = 0; c < 2; ++c) {
            int32_t temp = frame[c];
            temp = (temp > -32768) ? temp : -32768;
            temp = (temp < 32767) ? temp : 32767;
            output[c] = (int16_t)temp;
        }
        output += 2;
    }
    static_cast<T *>(this)->nativePostGenerate();
}

template <class T>
void OPNChipBaseT<T>::generateAndMix(int16_t *output, size_t frames)
{
    static_cast<T *>(this)->nativePreGenerate();
    for(size_t i = 0; i < frames; ++i)
    {
        int32_t frame[2];
        static_cast<T *>(this)->resampledGenerate(frame);
        for (unsigned c = 0; c < 2; ++c) {
            int32_t temp = (int32_t)output[c] + frame[c];
            temp = (temp > -32768) ? temp : -32768;
            temp = (temp < 32767) ? temp : 32767;
            output[c] = (int16_t)temp;
        }
        output += 2;
    }
    static_cast<T *>(this)->nativePostGenerate();
}

template <class T>
void OPNChipBaseT<T>::generate32(int32_t *output, size_t frames)
{
    static_cast<T *>(this)->nativePreGenerate();
    for(size_t i = 0; i < frames; ++i)
    {
        static_cast<T *>(this)->resampledGenerate(output);
        output += 2;
    }
    static_cast<T *>(this)->nativePostGenerate();
}

template <class T>
void OPNChipBaseT<T>::generateAndMix32(int32_t *output, size_t frames)
{
    static_cast<T *>(this)->nativePreGenerate();
    for(size_t i = 0; i < frames; ++i)
    {
        int32_t frame[2];
        static_cast<T *>(this)->resampledGenerate(frame);
        output[0] += frame[0];
        output[1] += frame[1];
        output += 2;
    }
    static_cast<T *>(this)->nativePostGenerate();
}

template <class T>
void OPNChipBaseT<T>::nativeTick(int16_t *frame)
{
#if defined(OPNMIDI_AUDIO_TICK_HANDLER)
    opn2_audioTickHandler(m_audioTickHandlerInstance, m_id, effectiveRate());
#endif
    static_cast<T *>(this)->nativeGenerate(frame);
}

template <class T>
void OPNChipBaseT<T>::setupResampler(uint32_t rate)
{
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    m_resampler->setup(rate * (1.0 / 53267), 2, 48);
#else
    m_oldsamples[0] = m_oldsamples[1] = 0;
    m_samples[0] = m_samples[1] = 0;
    m_samplecnt = 0;
    m_rateratio = (int32_t)(uint32_t)((((uint64_t)144 * rate) << rsm_frac) / m_clock);
#endif
}

template <class T>
void OPNChipBaseT<T>::resetResampler()
{
#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
    m_resampler->reset();
#else
    m_oldsamples[0] = m_oldsamples[1] = 0;
    m_samples[0] = m_samples[1] = 0;
    m_samplecnt = 0;
#endif
}

#if defined(OPNMIDI_ENABLE_HQ_RESAMPLER)
template <class T>
void OPNChipBaseT<T>::resampledGenerate(int32_t *output)
{
    if(UNLIKELY(m_runningAtPcmRate))
    {
        int16_t in[2];
        static_cast<T *>(this)->nativeTick(in);
        output[0] = (int32_t)in[0] * T::resamplerPreAmplify / T::resamplerPostAttenuate;
        output[1] = (int32_t)in[1] * T::resamplerPreAmplify / T::resamplerPostAttenuate;
        return;
    }

    VResampler *rsm = m_resampler;
    float scale = (float)T::resamplerPreAmplify /
        (float)T::resamplerPostAttenuate;
    float f_in[2];
    float f_out[2];
    rsm->inp_count = 0;
    rsm->inp_data = f_in;
    rsm->out_count = 1;
    rsm->out_data = f_out;
    while(rsm->process(), rsm->out_count != 0)
    {
        int16_t in[2];
        static_cast<T *>(this)->nativeTick(in);
        f_in[0] = scale * (float)in[0];
        f_in[1] = scale * (float)in[1];
        rsm->inp_count = 1;
        rsm->inp_data = f_in;
        rsm->out_count = 1;
        rsm->out_data = f_out;
    }
    output[0] = static_cast<int32_t>(std::lround(f_out[0]));
    output[1] = static_cast<int32_t>(std::lround(f_out[1]));
}
#else
template <class T>
void OPNChipBaseT<T>::resampledGenerate(int32_t *output)
{
    if(UNLIKELY(m_runningAtPcmRate))
    {
        int16_t in[2];
        static_cast<T *>(this)->nativeTick(in);
        output[0] = (int32_t)in[0] * T::resamplerPreAmplify / T::resamplerPostAttenuate;
        output[1] = (int32_t)in[1] * T::resamplerPreAmplify / T::resamplerPostAttenuate;
        return;
    }

    int32_t samplecnt = m_samplecnt;
    const int32_t rateratio = m_rateratio;
    while(samplecnt >= rateratio)
    {
        m_oldsamples[0] = m_samples[0];
        m_oldsamples[1] = m_samples[1];
        int16_t buffer[2];
        static_cast<T *>(this)->nativeTick(buffer);
        m_samples[0] = buffer[0] * T::resamplerPreAmplify;
        m_samples[1] = buffer[1] * T::resamplerPreAmplify;
        samplecnt -= rateratio;
    }
    output[0] = (int32_t)(((m_oldsamples[0] * (rateratio - samplecnt)
                            + m_samples[0] * samplecnt) / rateratio)/T::resamplerPostAttenuate);
    output[1] = (int32_t)(((m_oldsamples[1] * (rateratio - samplecnt)
                            + m_samples[1] * samplecnt) / rateratio)/T::resamplerPostAttenuate);
    m_samplecnt = samplecnt + (1 << rsm_frac);
}
#endif

/* OPNChipBaseBufferedT */

template <class T, unsigned Buffer>
void OPNChipBaseBufferedT<T, Buffer>::reset()
{
    OPNChipBaseT<T>::reset();
    m_bufferIndex = 0;
}

template <class T, unsigned Buffer>
void OPNChipBaseBufferedT<T, Buffer>::nativeGenerate(int16_t *frame)
{
    unsigned bufferIndex = m_bufferIndex;
    if(bufferIndex == 0)
        static_cast<T *>(this)->nativeGenerateN(m_buffer, Buffer);
    frame[0] = m_buffer[2 * bufferIndex];
    frame[1] = m_buffer[2 * bufferIndex + 1];
    bufferIndex = (bufferIndex + 1 < Buffer) ? (bufferIndex + 1) : 0;
    m_bufferIndex = bufferIndex;
}
