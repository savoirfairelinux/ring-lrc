/*
 *  Copyright (C) 2017-2018 Savoir-faire Linux Inc.
 *
 *  Author: Sébastien Blin <sebastien.blin@savoirfairelinux.com>
 *  Author: Andreas Traczyk <andreas.traczyk@savoirfairelinux.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 */

#include "waitforsignalhelper.h"

#include <QTimer>
#include <QSignalMapper>
#include <QtConcurrent/QtConcurrent>

WaitForSignalHelper::WaitForSignalHelper(QObject& object, const char* signal)
: QObject(), timeout_(false)
{
    connect(&object, signal, &eventLoop_, SLOT(quit()));
}

bool
WaitForSignalHelper::wait(unsigned int timeoutMs)
{
    QTimer timeoutHelper;
    if (timeoutMs != 0) {
        timeoutHelper.setInterval(timeoutMs);
        timeoutHelper.start();
        connect(&timeoutHelper, SIGNAL(timeout()), this, SLOT(timeout()));
    }
    timeout_ = false;
    eventLoop_.exec();
    return timeout_;
}

void
WaitForSignalHelper::timeout()
{
    timeout_ = true;
    eventLoop_.quit();
}

////////////////////////////////////////////////////////////////
WaitForSignalHelper::WaitForSignalHelper(std::function<void()> f)
:f_(f)
{
}

WaitForSignalHelper&
WaitForSignalHelper::addSignal(const std::string& id, QObject& object, const char* signal)
{
    results_.insert({id , false});
    QSignalMapper* signalMapper = new QSignalMapper(this);
    connect(&object, signal, signalMapper, SLOT(map()));
    signalMapper->setMapping(&object, QString::fromStdString(id));
    connect(signalMapper,
            SIGNAL(mapped(const QString&)),
            this,
            SLOT(signalSlot(const QString&)));
    return *this;
}

void
WaitForSignalHelper::signalSlot(const QString & id)
{
    std::string signalId = id.toStdString();
    printf("Signal caught: %s\n", signalId.c_str());
    auto resultsSize = results_.size();
    unsigned signalsCaught = 0;
    // loop through results map till we find the signal id and set the value to
    // true, meanwhile testing the total caught signals and exiting the wait loop
    // if all the signals have come through
    for (auto it = results_.begin(); it != results_.end(); it++) {
        if ((*it).first.compare(signalId) == 0) {
            (*it).second = true;
        }
        signalsCaught = signalsCaught + static_cast<unsigned>((*it).second);
    }
    if (signalsCaught == resultsSize) {
        printf("All signals caught\n");
        isRunning_.store(false);
        eventLoop_.quit();
    }
}

void
WaitForSignalHelper::timeout2()
{
    printf("Timed out! signal(s) missed\n");
    isRunning_.store(false);
    eventLoop_.quit();
}

std::map<std::string, bool>
WaitForSignalHelper::wait2(int timeoutMs)
{
    if (timeoutMs <= 0) {
        throw std::invalid_argument("Invalid time out value");
    }
    // connect timer to A::timeout() here… or use chrono and busy loop, cv, etc.
    QTimer timeoutHelper;
    timeoutHelper.setInterval(timeoutMs);
    timeoutHelper.start();
    connect(&timeoutHelper, SIGNAL(timeout()), this, SLOT(timeout2()));
    printf("Starting %dms wait...\n", timeoutMs);
    auto resultsFuture = QtConcurrent::run([&]() -> void {
        // block with eventLoop_ and fill timeout map
        cv_.notify_all();
        eventLoop_.exec();
        return;
    });
    // wait till ready or timeout
    std::thread([this, timeoutMs] () {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait_for(lk, std::chrono::milliseconds(timeoutMs), [this, timeoutMs] {
            auto start = std::chrono::high_resolution_clock::now();
            while (!eventLoop_.isRunning()) {}
            return true;
        });
    }).join();
    // execute function expected to produce awaited signal(s)
    f_();
    // wait for results… if they come, else time out
    resultsFuture.waitForFinished();
    printf("Done waiting\n");
    return results_;
}
////////////////////////////////////////////////////////////////
