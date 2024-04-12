//
// Created by matin on 11/04/24.
//

#ifndef GSTREAMER1_0_DRPAI_THREAD_H
#define GSTREAMER1_0_DRPAI_THREAD_H

#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

struct split_thread {
    std::thread *t;
    GstElement *e;
    bool thread_stop = false;

    explicit split_thread(GstElement *splitmuxsink): e(splitmuxsink) {
        if (splitmuxsink)
            t = new std::thread(&split_thread::worker_thread, this);
    }

    void stop() {
        if (t) {
            g_print("Stopping split thread..\n");
            thread_stop = true;
            t->join();
            delete t;
        }
    }

private:
    void worker_thread() {
        g_print("Started the split timer thread.\n");
        using namespace std::chrono_literals;
        using namespace std::chrono;
        while (!thread_stop) {
            auto tp = system_clock::now(); // date_time in UTC
            auto tp_days = time_point_cast<duration<int, std::ratio<86400>>>(tp); // date in UTC
            auto tod = tp - tp_days; // time in UTC

            // separate a duration into {h, m, s}
            auto h = duration_cast<hours>(tod);
            tod -= h;
            auto m = duration_cast<minutes>(tod);
            tod -= m;
            auto s = duration_cast<seconds>(tod);
            if (m.count() == 0 && s.count() == 0) {
                std::time_t tt = std::chrono::system_clock::to_time_t(tp);
                std::tm tm = *std::gmtime(&tt);
                std::stringstream ss;
                ss << std::put_time( &tm, "%Y-%m-%d %H:%M:%S UTC" );
                g_print("Emitted signal in split thread at %s\n", ss.str().c_str());
                g_signal_emit_by_name(e, "split-now");
                std::this_thread::sleep_for(1s);
            }
            std::this_thread::sleep_for(1s);
        }
    }
};

#endif //GSTREAMER1_0_DRPAI_THREAD_H
