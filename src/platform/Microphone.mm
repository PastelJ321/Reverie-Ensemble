// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#import <AVFoundation/AVFoundation.h>
#include <QEventLoop>
#include <QTimer>
#include <atomic>
#include <memory>
namespace reverie {
bool requestMicrophonePermission() {
    auto status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
    if (status == AVAuthorizationStatusAuthorized)
        return true;
    if (status == AVAuthorizationStatusDenied || status == AVAuthorizationStatusRestricted)
        return false;
    auto done = std::make_shared<std::atomic<int>>(0);
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio
                             completionHandler:^(BOOL allowed) {
                               done->store(allowed ? 1 : -1);
                             }];
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, [&] {
        if (done->load() != 0)
            loop.quit();
    });
    timer.start(30);
    loop.exec();
    return done->load() == 1;
}
} // namespace reverie
