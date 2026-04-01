#pragma once
#include <cstdint>
#include <functional>
#include <string>

// RTL-SDR wrapper supporting both async streaming and synchronous hop reads.
class SdrDevice {
public:
    using Callback = std::function<void(const uint8_t* buf, uint32_t len)>;

    SdrDevice()  = default;
    ~SdrDevice() { close(); }

    SdrDevice(const SdrDevice&)            = delete;
    SdrDevice& operator=(const SdrDevice&) = delete;

    // Open/close device (idempotent).
    bool open(uint32_t sample_rate, int device_index = 0);
    void close();

    // Change center frequency at any time after open().
    bool set_center_freq(uint32_t freq_hz);

    // Synchronous read: exactly buf_size bytes (must be multiple of 512).
    // Returns number of bytes read, or -1 on error.
    int  read_sync(uint8_t* buf, int buf_size);

    // Async streaming (legacy single-freq convenience path).
    bool start_async(uint32_t freq_hz, uint32_t sample_rate, Callback cb, int device_index = 0);
    void stop_async();

    bool        is_open()  const { return m_dev != nullptr; }
    std::string error()    const { return m_error; }

private:
    static void rtlsdr_callback(uint8_t* buf, uint32_t len, void* ctx);

    void*       m_dev{nullptr};
    Callback    m_callback;
    bool        m_async_running{false};
    std::string m_error;
};
