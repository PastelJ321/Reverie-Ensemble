// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "platform/MidiInput.h"
#include <array>
#include <atomic>
#include <mutex>
#ifdef __APPLE__
#include <CoreMIDI/CoreMIDI.h>
#include <mach/mach_time.h>
#endif
namespace reverie {
struct MidiInput::Impl {
    std::array<MidiMessage, 4096> queue{};
    unsigned read = 0, write = 0;
    // CoreMIDI may call separate source callbacks concurrently. No audio thread takes this lock.
    std::mutex mutex;
    std::atomic<bool> overflow{false}, changed{false};
    std::int32_t selected = -1;
#ifdef __APPLE__
    MIDIClientRef client = 0;
    MIDIPortRef port = 0;
    struct Source {
        MIDIEndpointRef endpoint;
        std::int32_t id;
        std::uint8_t running = 0, pending = 0, first = 0;
        int used = 0;
        bool sysex = false;
    };
    std::array<Source, 128> sources{};
    unsigned sourceCount = 0;
    void push(MidiMessage m) noexcept {
        auto n = (write + 1) % queue.size();
        if (n == read) {
            overflow = true;
            return;
        }
        queue[write] = m;
        write = unsigned(n);
    }
    static void receive(const MIDIPacketList* list, void* context, void* ref) {
        auto& self = *static_cast<Impl*>(context);
        auto& source = *static_cast<Source*>(ref);
        std::lock_guard lock(self.mutex);
        auto* packet = &list->packet[0];
        for (unsigned p = 0; p < list->numPackets; ++p, packet = MIDIPacketNext(packet))
            for (unsigned i = 0; i < packet->length; ++i) {
                auto b = packet->data[i];
                if (b >= 0xf8)
                    continue;
                if (b == 0xf0) {
                    source.sysex = true;
                    source.running = 0;
                    continue;
                }
                if (b == 0xf7) {
                    source.sysex = false;
                    continue;
                }
                if (source.sysex)
                    continue;
                if (b & 0x80) {
                    source.running = b < 0xf0 ? b : 0;
                    source.used = 0;
                    continue;
                }
                if (!source.running)
                    continue;
                int size =
                    ((source.running & 0xf0) == 0xc0 || (source.running & 0xf0) == 0xd0) ? 1 : 2;
                if (source.used++ == 0)
                    source.first = b;
                if (source.used == size) {
                    self.push({packet->timeStamp ? packet->timeStamp : mach_absolute_time(),
                               source.running, source.first, std::uint8_t(size == 2 ? b : 0),
                               source.id});
                    source.used = 0;
                }
            }
    }
    void refresh() {
        if (!port)
            return;
        for (unsigned i = 0; i < sourceCount; ++i)
            MIDIPortDisconnectSource(port, sources[i].endpoint);
        std::lock_guard lock(mutex);
        for (auto& s : sources) {
            s.running = 0;
            s.used = 0;
            s.sysex = false;
        }
        for (ItemCount i = 0; i < MIDIGetNumberOfSources(); ++i) {
            auto endpoint = MIDIGetSource(i);
            SInt32 id = 0;
            MIDIObjectGetIntegerProperty(endpoint, kMIDIPropertyUniqueID, &id);
            if (selected == 0 || (selected != -1 && id != selected))
                continue;
            Source* entry = nullptr;
            for (unsigned n = 0; n < sourceCount; ++n)
                if (sources[n].endpoint == endpoint) {
                    entry = &sources[n];
                    break;
                }
            if (!entry) {
                if (sourceCount >= sources.size())
                    continue;
                entry = &sources[sourceCount++];
                *entry = {endpoint, id};
            }
            MIDIPortConnectSource(port, endpoint, entry);
        }
        read = write;
    }
#endif
};
MidiInput::MidiInput() : impl_(std::make_unique<Impl>()) {
#ifdef __APPLE__
    MIDIClientCreateWithBlock(CFSTR("Reverie Ensemble MIDI"), &impl_->client,
                              ^(const MIDINotification*) {
                                impl_->changed = true;
                              });
    MIDIInputPortCreate(impl_->client, CFSTR("Performance input"), Impl::receive, impl_.get(),
                        &impl_->port);
    impl_->refresh();
#endif
}
MidiInput::~MidiInput() {
#ifdef __APPLE__
    if (impl_->port)
        MIDIPortDispose(impl_->port);
    if (impl_->client)
        MIDIClientDispose(impl_->client);
#endif
}
std::vector<MidiDevice> MidiInput::devices() {
    std::vector<MidiDevice> result;
#ifdef __APPLE__
    for (ItemCount i = 0; i < MIDIGetNumberOfSources(); ++i) {
        auto endpoint = MIDIGetSource(i);
        SInt32 id = 0;
        MIDIObjectGetIntegerProperty(endpoint, kMIDIPropertyUniqueID, &id);
        CFStringRef name = nullptr;
        MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName, &name);
        char buffer[1024]{};
        if (name) {
            CFStringGetCString(name, buffer, sizeof(buffer), kCFStringEncodingUTF8);
            CFRelease(name);
        }
        result.push_back({id, buffer});
    }
#endif
    return result;
}
void MidiInput::select(std::int32_t id) {
    impl_->selected = id;
#ifdef __APPLE__
    impl_->refresh();
#endif
}
bool MidiInput::pop(MidiMessage& m) noexcept {
    std::lock_guard lock(impl_->mutex);
    if (impl_->read == impl_->write)
        return false;
    m = impl_->queue[impl_->read];
    impl_->read = (impl_->read + 1) % impl_->queue.size();
    return true;
}
void MidiInput::inject(MidiMessage m) noexcept {
    std::lock_guard lock(impl_->mutex);
    auto n = (impl_->write + 1) % impl_->queue.size();
    if (n == impl_->read) {
        impl_->overflow = true;
        return;
    }
    impl_->queue[impl_->write] = m;
    impl_->write = unsigned(n);
}
bool MidiInput::takeOverflow() noexcept {
    return impl_->overflow.exchange(false);
}
bool MidiInput::takeDevicesChanged() noexcept {
    bool result = impl_->changed.exchange(false);
#ifdef __APPLE__
    if (result)
        impl_->refresh();
#endif
    return result;
}
double MidiInput::secondsSince(std::uint64_t stamp) const noexcept {
#ifdef __APPLE__
    if (!stamp)
        return 0;
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    auto now = mach_absolute_time();
    return now >= stamp ? double(now - stamp) * info.numer / info.denom * 1e-9 : 0;
#else
    (void)stamp;
    return 0;
#endif
}
} // namespace reverie
