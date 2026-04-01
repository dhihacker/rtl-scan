#include "SdrDevice.h"
#include <rtl-sdr.h>
#include <cstdio>
#include <thread>

static rtlsdr_dev_t* dev_cast(void* p) { return static_cast<rtlsdr_dev_t*>(p); }

bool SdrDevice::open(uint32_t sample_rate, int device_index) {
    if (m_dev) return true;  // already open

    rtlsdr_dev_t* dev = nullptr;
    if (rtlsdr_open(&dev, device_index) < 0) {
        m_error = "Failed to open RTL-SDR device";
        return false;
    }
    rtlsdr_set_sample_rate(dev, sample_rate);
    rtlsdr_set_tuner_gain_mode(dev, 0);  // auto gain
    rtlsdr_reset_buffer(dev);
    m_dev = dev;
    return true;
}

void SdrDevice::close() {
    stop_async();
    if (m_dev) {
        rtlsdr_close(dev_cast(m_dev));
        m_dev = nullptr;
    }
}

bool SdrDevice::set_center_freq(uint32_t freq_hz) {
    if (!m_dev) return false;
    rtlsdr_set_center_freq(dev_cast(m_dev), freq_hz);
    rtlsdr_reset_buffer(dev_cast(m_dev));
    return true;
}

int SdrDevice::read_sync(uint8_t* buf, int buf_size) {
    if (!m_dev) return -1;
    int n_read = 0;
    if (rtlsdr_read_sync(dev_cast(m_dev), buf, buf_size, &n_read) < 0)
        return -1;
    return n_read;
}

bool SdrDevice::start_async(uint32_t freq_hz, uint32_t sample_rate, Callback cb, int device_index) {
    if (!open(sample_rate, device_index)) return false;
    m_callback = std::move(cb);
    set_center_freq(freq_hz);
    m_async_running = true;
    std::thread([this]() {
        rtlsdr_read_async(dev_cast(m_dev), rtlsdr_callback, this, 0, 0);
        m_async_running = false;
    }).detach();
    return true;
}

void SdrDevice::stop_async() {
    if (m_async_running && m_dev) {
        rtlsdr_cancel_async(dev_cast(m_dev));
        m_async_running = false;
    }
}

void SdrDevice::rtlsdr_callback(uint8_t* buf, uint32_t len, void* ctx) {
    auto* self = static_cast<SdrDevice*>(ctx);
    if (self->m_callback)
        self->m_callback(buf, len);
}
