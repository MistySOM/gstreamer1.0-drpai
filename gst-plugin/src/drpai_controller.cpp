//
// Created by matin on 21/02/23.
//

#include "drpai_controller.h"
#include "drpai-models/drpai-yolo/yolo_post_processor.h"
#ifdef ENABLE_TVM
#include "drpai-models/drpai-tvm/tvm_drpai.h"
#endif
#include <memory>
#include <iostream>
#include <netdb.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <chrono>


void DRPAI_Controller::open_resources() {
    if (drpai->rate.get_max_rate() == 0) {
        std::cout << "[WARNING] DRPAI is disabled by the zero max framerate." << std::endl;
        return;
    }

    if (multithread)
        process_thread = std::make_unique<std::thread>(&DRPAI_Controller::thread_function_loop, this);
    else
        thread_state = Ready;

    /* Obtain udmabuf memory area starting address */
    uint64_t udmabuf_address;
    {
        std::ifstream file ("/sys/class/u-dma-buf/udmabuf0/phys_addr", std::ifstream::in);
        if (!file.is_open())
            throw std::runtime_error("[ERROR] Failed to open udmabuf0/phys_addr : errno="  + std::string(std::strerror(errno)));
        file >> std::hex >> udmabuf_address;
        file.close();
    }
    /* Filter the bit higher than 32 bit */
    udmabuf_address &=0xFFFFFFFF;

    /**********************************************************************/
    /* Inference preparation                                              */
    /**********************************************************************/

    /* Read DRP-AI Object files address and size */
    drpai->open_resource(udmabuf_address, true);
    std::cout <<"DRP-AI Ready!" << std::endl;
}

void DRPAI_Controller::process_image(uint8_t* img_data, uint32_t img_data_len) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    if (drpai->rate.get_max_rate() != 0) {
        switch (thread_state) {
            case Failed:
            case Unknown:
            case Closing:
                throw std::exception();

            case Ready:
                image_mapped_udma->copy(img_data, img_data_len, BGR_DATA);
                //std::this_thread::sleep_for(std::chrono::milliseconds(50));
                thread_state = Processing;
                if (multithread)
                    v.notify_one();

            case Processing:
            default:
                break;
        }
    }
    const auto t1 = std::chrono::high_resolution_clock::now();

    if(drpai->rate.get_max_rate() != 0 && !multithread)
        try {
            thread_state = Ready;
            thread_function_single();
        }
        catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            thread_state = Failed;
            throw;
        }

    Image img (image_mapped_udma->img_w, image_mapped_udma->img_h, image_mapped_udma->img_c, BGR_DATA, img_data);
    video_rate.inform_frame();
    const auto t2 = std::chrono::high_resolution_clock::now();

    /* Compute the result, draw the result on img and display it on console */
    std::vector<std::string> corner_text {};
    if (show_time) {
        const auto now = std::chrono::system_clock::now();
        const auto now_t = std::chrono::system_clock::to_time_t(now);
        const auto now_l = std::localtime(&now_t);
        char now_str[25];
        snprintf( now_str, 25, "Current Time: %02d:%02d:%02d", now_l->tm_hour, now_l->tm_min, now_l->tm_sec);
        corner_text.emplace_back(now_str);
    }
    if (show_fps) {
        corner_text.push_back("Video Rate: " + std::to_string(static_cast<int32_t>(video_rate.get_smooth_rate())) + " fps");
        corner_text.push_back(drpai->get_status());
        corner_text.push_back(postprocessor->get_status());
        if (det_tracker.active) {
            corner_text.push_back("Tracked/" + std::to_string(det_tracker.history_length/60) +
                "min: " + std::to_string(det_tracker.count()));
        }
    }
    if (show_filter) {
        det_filterer.render_filter_region(img);
    }
    if (show_bbox) {
        if (det_tracker.active) {
            for (const auto& tracked: det_tracker.last_tracked_detection) {
                img.draw_rect(tracked->smooth_bbox.mix, tracked->to_string_hr(show_track_id, labels));
            }
        } else {
            postprocessor->detections.draw(img, labels);
        }
    }
    img.render_text_at_corner(corner_text);
    const auto t3 = std::chrono::high_resolution_clock::now();

    if (log_exec_time) {
        const auto ms_int0 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        const auto ms_int1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        const auto ms_int2 = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
        std::cout << std::endl << "Video Frame Times:" << std::endl;
        std::cout << "UDMA copy:\t" << ms_int0 << "ms" << std::endl;
        std::cout << "Image define:\t" << ms_int1 << "ms"<< std::endl;
        std::cout << "Render bbox:\t" << ms_int2 << "ms"<< std::endl;
    }
}

void DRPAI_Controller::set_socket_address(const std::string& address) {
    const auto& colon_index = address.find(':');
    const auto host = address.substr(0, colon_index);
    const auto port = address.substr(colon_index+1);
    constexpr addrinfo hints {0,  AF_INET, SOCK_DGRAM};

    addrinfo* result = nullptr;
    int r;
    if ((r = getaddrinfo(host.c_str(), port.c_str(), &hints, &result)) != 0) {
        freeaddrinfo(result);
        std::cerr << "[Warning] Can't resolve " << address << ": " << gai_strerror(r) << std::endl;
        return;
    }

    /* getaddrinfo() returns a list of address structures.
        Try each address until we successfully connect(2).
        If socket(2) (or connect(2)) fails, we (close the socket and) try the next address. */

    addrinfo* rp;
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_fd > 0)
            break;
    }
    if (rp == nullptr) { /* No address succeeded */
        std::cerr << "[Warning] Can't connect to " + address << std::endl;
        socket_fd = 0;
        freeaddrinfo(result);
        return;
    }

    std::memcpy(&socket_address, rp->ai_addr, rp->ai_addrlen);
    freeaddrinfo(result);

    std::cout << "Option: Sending UDP packets to " << address << std::endl;
}

void DRPAI_Controller::open_resources_with_image_size(uint16_t image_width, uint16_t image_height) {

    if (drpai->IN_WIDTH != image_width || drpai->IN_HEIGHT != image_height)
        throw std::runtime_error(std::string("[ERROR] The model only supports image input with resolution ") +
            std::to_string(drpai->IN_WIDTH) + "x" + std::to_string(drpai->IN_HEIGHT));

    image_mapped_udma = std::make_unique<Image>(image_width, image_height, drpai->IN_CHANNEL, drpai->IN_FORMAT, nullptr);
    image_mapped_udma->map_udmabuf();

    postprocessor->open_resource(drpai->drpai_output_buf.size(),
        image_width, image_height, labels.size());

    if (det_filterer.is_filter_region_active())
        std::cout << "Option : Filtering region of interest to " << det_filterer.get_filter_region_json().to_string() << std::endl;
    else {
        det_filterer.set_filter_region_width(image_width);
        det_filterer.set_filter_region_height(image_height);
    }
}

void DRPAI_Controller::release_resources() {
    if(process_thread) {
        {
            std::unique_lock<std::mutex> state_lock(state_mutex);
            thread_state = Closing;
            v.notify_one();
        }
        process_thread->join();
        process_thread.reset();
    }

    drpai->release_resource();
    image_mapped_udma.reset();
    dlclose(dynamic_library_handle);
}

void DRPAI_Controller::thread_function_loop() {
    try {
        pthread_setname_np(pthread_self(), "drpai_thread");
        while (true) {
            thread_function_single();
        }
    }
    catch (const std::runtime_error& e) {
        std::cerr << "[Loop ERROR] " << e.what() << std::endl;
        thread_state = Failed;
    }
    catch (const std::exception& e) {
        if (thread_state != Closing) {
            std::cerr << "[Loop ERROR] " << e.what() << std::endl;
            thread_state = Failed;
        }
    }
}

void DRPAI_Controller::thread_function_single() {
    try {
        const auto t0 = std::chrono::high_resolution_clock::now();
        {
            std::unique_lock lock(state_mutex);
            if (thread_state == Closing)
                throw std::exception();

            thread_state = Ready;
            if (multithread) {
                v.wait(lock, [&] { return thread_state != Ready; });
            }
        }
        const auto t1 = std::chrono::high_resolution_clock::now();

        image_mapped_udma->prepare();
        const auto t2 = std::chrono::high_resolution_clock::now();

        drpai->run_inference();
        const auto t3 = std::chrono::high_resolution_clock::now();

        postprocessor->extract_detections(drpai->drpai_output_buf);
        const auto t4 = std::chrono::high_resolution_clock::now();

        det_filterer.apply(postprocessor->detections.get_current());
        if(det_tracker.active) {
            det_tracker.track(postprocessor->detections.get_current());
        }
        postprocessor->detections.submit_current();
        const auto t5 = std::chrono::high_resolution_clock::now();

        check_save_bmp();
        const auto t6 = std::chrono::high_resolution_clock::now();

        send_socket_data();
        const auto t7 = std::chrono::high_resolution_clock::now();

        /* Print details */
        if(log_detects) {
            if (det_tracker.active) {
                det_tracker.print_string_hr(labels);
            } else {
                postprocessor->detections.print_string_hr(labels);
            }
        }

        if (log_exec_time) {
            const auto ms_int0 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            const auto ms_int1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            const auto ms_int2 = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
            const auto ms_int3 = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
            const auto ms_int4 = std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4).count();
            const auto ms_int5 = std::chrono::duration_cast<std::chrono::milliseconds>(t6 - t5).count();
            const auto ms_int6 = std::chrono::duration_cast<std::chrono::milliseconds>(t7 - t6).count();
            std::cout << std::endl << "Execution Times:" << std::endl;
            std::cout << "Lock:\t" << ms_int0 << "ms" << std::endl;
            std::cout << "UDMA prepare:\t" << ms_int1 << "ms"<< std::endl;
            std::cout << "Inference:\t" << ms_int2 << "ms"<< std::endl;
            drpai->print_log_exec_time();
            std::cout << "Extract:\t" << ms_int3 << "ms"<< std::endl;
            std::cout << "Filter & Track:\t" << ms_int4 << "ms"<< std::endl;
            std::cout << "SaveBMP:\t" << ms_int5 << "ms"<< std::endl;
            std::cout << "SendSocket:\t" << ms_int6 << "ms"<< std::endl;
        }

        if (error_retries > 0) {
            std::cout << "DRPAI recovered after retrying." << std::endl;
            error_retries = 0;
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        error_retries++;
        if (error_retries >= 3) {
            throw std::runtime_error("thread failed 3 consequent times. Letting the GStreamer know.");
        }
    }
}

DRPAI_Controller::DRPAI_Controller():
    det_tracker(true, 2, 2.25, 1)
{
}

void DRPAI_Controller::open_post_processor_library(const std::string &modelPrefix) {
    const std::string params_file_name = modelPrefix + "/" + modelPrefix + "_post_process_params.txt";
    const std::string model_library_path = BasePostProcessor::get_param(params_file_name, "[dynamic_library]", true);

    std::cout << "Loading : " << model_library_path << std::endl;
    dynamic_library_handle = dlopen(model_library_path.c_str(), RTLD_NOW);
    if (!dynamic_library_handle)
        throw std::runtime_error("[ERROR] Failed to open library " + std::string(dlerror()));

    dlerror();    /* Clear any existing error */
    const auto create_post_processor_instance_dl = reinterpret_cast<create_post_processor_instance_def>(dlsym(dynamic_library_handle, "create_post_processor_instance"));
    if (const auto error = dlerror(); error != nullptr)
        throw std::runtime_error("[ERROR] Failed to locate function in " + model_library_path + ": error=" + error);

    postprocessor = (*create_post_processor_instance_dl)(modelPrefix.c_str());
}

void DRPAI_Controller::set_property(GstDRPAI_Properties prop, const GValue *value) {
    switch (prop) {
        case PROP_MULTITHREAD:
            multithread = g_value_get_boolean(value);
            break;
        case PROP_LOG_SERVER:
            set_socket_address(g_value_get_string(value));
            break;
        case PROP_SHOW_FPS:
            show_fps = g_value_get_boolean(value);
            break;
        case PROP_SHOW_TIME:
            show_time = g_value_get_boolean(value);
            break;
        case PROP_SHOW_BBOX:
            show_bbox = g_value_get_boolean(value);
            break;
        case PROP_MAX_VIDEO_RATE:
            video_rate.set_max_rate(g_value_get_float(value));
            break;
        case PROP_SMOOTH_VIDEO_RATE:
            video_rate.set_smooth_rate(g_value_get_uint(value));
            break;
        case PROP_MODEL: {
            const auto prefix = std::string(g_value_get_string(value));
            if (std::ifstream(prefix + "/deploy.so").good()) {
#ifdef ENABLE_TVM
                drpai = new TVM_DRPAI(prefix);
#else
                throw std::runtime_error("This built of gstreamer1.0-drpai plugin doesn't support TVM deployment models.");
#endif
            } else {
                drpai = new BaseDRPAI(prefix);
            }

            /*Load Label from label_list file*/
            const std::string label_list = prefix + "/" + prefix + "_labels.txt";
            std::cout << "Loading : " << label_list << std::flush;
            load_label_file(label_list);
            std::cout << "\t\t\tFound classes: " << labels.size() << std::endl;

            open_post_processor_library(prefix);
            break;
        }
        case PROP_LOG_DETECTS:
            log_detects = g_value_get_boolean(value);
            break;
        case PROP_LOG_EXEC_TIME:
            log_exec_time = g_value_get_boolean(value);
            break;
        case PROP_TRACKING:
            det_tracker.active = g_value_get_boolean(value);
            if (det_tracker.active) {
                std::cout << "Option : Detection Tracking is Active!" << std::endl;
            }
            break;
        case PROP_SHOW_TRACK_ID:
            show_track_id = g_value_get_boolean(value);
            break;
        case PROP_SMOOTH_BBOX_RATE:
            det_tracker.bbox_smooth_rate = g_value_get_uint(value);
            break;
        case PROP_HISTORY_LENGTH:
            det_tracker.history_length = g_value_get_uint(value)*60;
            break;
        case PROP_TRACK_SECONDS:
            det_tracker.time_threshold = g_value_get_float(value);
            break;
        case PROP_DOA_THRESHOLD:
            det_tracker.doa_threshold = g_value_get_float(value);
            break;
        case PROP_FILTER_NMS:
            det_filterer.TH_NMS = static_cast<float>(g_value_get_uint(value)) / 100.f;
            break;
        case PROP_FILTER_PROB:
            postprocessor->TH_PROB = static_cast<float>(g_value_get_uint(value)) / 100.f;
            break;
        case PROP_FILTER_SHOW:
            show_filter = g_value_get_boolean(value);
            break;
        case PROP_FILTER_CLASS:
            det_filterer.set_filter_classes(labels,g_value_get_string(value));
            break;
        case PROP_FILTER_LEFT:
            det_filterer.set_filter_region_left(static_cast<float>(g_value_get_uint(value)));
            break;
        case PROP_FILTER_TOP:
            det_filterer.set_filter_region_top(static_cast<float>(g_value_get_uint(value)));
            break;
        case PROP_FILTER_WIDTH:
            det_filterer.set_filter_region_width(static_cast<float>(g_value_get_uint(value)));
            break;
        case PROP_FILTER_HEIGHT:
            det_filterer.set_filter_region_height(static_cast<float>(g_value_get_uint(value)));
            break;
        case PROP_BITMAP_SAVE_DIR:
            bitmap_save_directory = g_value_get_string(value);
            break;
        case PROP_BITMAP_SAVE_MINUTES:
            bitmap_save_time_between = g_value_get_float(value);
            break;
        case PROP_BITMAP_SAVE_PROB:
            bitmap_save_class_probability = g_value_get_float(value)/100.f;
            break;
        case PROP_BITMAP_SAVE_CLASS: {
            bitmap_save_classes.clear();
            std::stringstream ss (g_value_get_string(value));
            std::string item;
            while (getline(ss, item, ',')) {
                bitmap_save_classes.push_back (item);
            }
            break;
        }
        default:
            drpai->set_property(prop, value);
            break;
    }
}

void DRPAI_Controller::get_property(GstDRPAI_Properties prop, GValue *value) const {
    switch (prop) {
        case PROP_MULTITHREAD:
            g_value_set_boolean(value, multithread);
            break;
        case PROP_SHOW_FPS:
            g_value_set_boolean(value, show_fps);
            break;
        case PROP_SHOW_TIME:
            g_value_set_boolean(value, show_time);
            break;
        case PROP_SHOW_BBOX:
            g_value_set_boolean(value, show_bbox);
            break;
        case PROP_MAX_VIDEO_RATE:
            g_value_set_float(value, video_rate.get_max_rate());
            break;
        case PROP_SMOOTH_VIDEO_RATE:
            g_value_set_uint(value, video_rate.get_max_smooth_rate());
            break;
        case PROP_LOG_DETECTS:
            g_value_set_boolean(value, log_detects);
            break;
        case PROP_LOG_EXEC_TIME:
            g_value_set_boolean(value, log_exec_time);
            break;
        case PROP_TRACKING:
            g_value_set_boolean(value, det_tracker.active);
            break;
        case PROP_SHOW_TRACK_ID:
            g_value_set_boolean(value, show_track_id);
            break;
        case PROP_SMOOTH_BBOX_RATE:
            g_value_set_uint(value, det_tracker.bbox_smooth_rate);
            break;
        case PROP_HISTORY_LENGTH:
            g_value_set_uint(value, det_tracker.history_length/60);
            break;
        case PROP_TRACK_SECONDS:
            g_value_set_float(value, det_tracker.time_threshold);
            break;
        case PROP_DOA_THRESHOLD:
            g_value_set_float(value, det_tracker.doa_threshold);
            break;
        case PROP_FILTER_NMS:
            g_value_set_uint(value, static_cast<uint>(det_filterer.TH_NMS * 100));
            break;
        case PROP_FILTER_PROB:
            g_value_set_uint(value, static_cast<uint>(postprocessor->TH_PROB * 100));
            break;
        case PROP_FILTER_SHOW:
            g_value_set_boolean(value, show_filter);
            break;
        case PROP_FILTER_CLASS:
            g_value_set_string(value, det_filterer.get_filter_classes_string(labels).c_str());
            break;
        case PROP_FILTER_LEFT:
            g_value_set_uint(value, static_cast<uint>(det_filterer.get_filter_region_left()));
            break;
        case PROP_FILTER_TOP:
            g_value_set_uint(value, static_cast<uint>(det_filterer.get_filter_region_top()));
            break;
        case PROP_FILTER_WIDTH:
            g_value_set_uint(value, static_cast<uint>(det_filterer.get_filter_region_width()));
            break;
        case PROP_FILTER_HEIGHT:
            g_value_set_uint(value, static_cast<uint>(det_filterer.get_filter_region_height()));
            break;
        case PROP_BITMAP_SAVE_DIR:
            g_value_set_string(value, bitmap_save_directory.c_str());
            break;
        case PROP_BITMAP_SAVE_MINUTES:
            g_value_set_float(value, bitmap_save_time_between);
            break;
        case PROP_BITMAP_SAVE_PROB:
            g_value_set_float(value, bitmap_save_class_probability*100);
            break;
        case PROP_BITMAP_SAVE_CLASS: {
            std::string s;
            if (!bitmap_save_classes.empty()) {
                for (const auto& item: bitmap_save_classes)
                    s += item + ",";
                s.pop_back();
            }
            g_value_set_string(value, s.c_str());
            break;
        }
        default:
            drpai->get_property(prop, value);
            break;
    }
}

void DRPAI_Controller::install_properties(std::map<GstDRPAI_Properties, _GParamSpec*>& params) {
    params.emplace(PROP_MODEL, g_param_spec_string(
        "model", "Model",
        "The name of the pretrained model and the directory prefix.",
        nullptr, G_PARAM_READWRITE));
    params.emplace(PROP_LOG_DETECTS, g_param_spec_boolean(
        "log_detects", "Log Detects",
        "Print detected objects in standard output.",
        FALSE, G_PARAM_READWRITE));
    params.emplace(PROP_LOG_EXEC_TIME, g_param_spec_boolean(
        "log_exec_time", "Log Execution Time",
        "Print execution time into the standard output.",
        FALSE, G_PARAM_READWRITE));
    params.emplace(PROP_MULTITHREAD, g_param_spec_boolean(
        "multithread", "MultiThread",
        "Use a separate thread for object detection.",
        TRUE, G_PARAM_READWRITE));
    params.emplace(PROP_SHOW_FPS, g_param_spec_boolean(
        "show_fps", "Show Frame Rates",
        "Render frame rates of video and DRPAI at the corner of the video.",
        FALSE, G_PARAM_READWRITE));
    params.emplace(PROP_SHOW_TIME, g_param_spec_boolean(
        "show_time", "Show Current Time",
        "Render current time at the corner of the video.",
        FALSE, G_PARAM_READWRITE));
    params.emplace(PROP_SHOW_BBOX, g_param_spec_boolean(
        "show_bbox", "Show Bounding Boxes",
        "Render the latest detection bounding boxes on the video.",
        TRUE, G_PARAM_READWRITE));
    params.emplace(PROP_MAX_VIDEO_RATE, g_param_spec_float(
        "max_video_rate", "Max Video Framerate",
        "Force maximum video frame rate using thread sleeps.",
        0.001f, 120.f, 120.f, G_PARAM_READWRITE));
    params.emplace(PROP_SMOOTH_VIDEO_RATE, g_param_spec_uint(
        "smooth_video_rate", "Smooth Video Framerate",
        "Number of last video frame rates to average for a more smooth value.",
        1, 1000, 1, G_PARAM_READWRITE));
    params.emplace(PROP_LOG_SERVER, g_param_spec_string(
        "log_server", "Log Server",
        "Send UDP messages in JSON about detected objects to the mentioned host:port.",
        nullptr, G_PARAM_WRITABLE));

    params.emplace(PROP_TRACKING, g_param_spec_boolean(
        "tracking", "Tracking",
        "Track detected objects based on their previous locations. Each detected object gets an ID that persists across multiple detections based on other tracking properties.",
        true, G_PARAM_READWRITE));
    params.emplace(PROP_SHOW_TRACK_ID, g_param_spec_boolean(
        "show-track-id", "Show Track ID",
        "Show the track ID on the detection labels.",
        false, G_PARAM_READWRITE));
    params.emplace(PROP_TRACK_SECONDS, g_param_spec_float(
        "track-seconds", "Track Seconds",
        "Number of seconds to wait for a tracked undetected object to forget it.",
        0.001, 100, 2, G_PARAM_READWRITE));
    params.emplace(PROP_DOA_THRESHOLD, g_param_spec_float(
        "track-doa-threshold", "Track DOA Threashold",
        "The threshold of Distance Over Areas (DOA) for tracking bounding-boxes.",
        0.001, 1000, 2.25, G_PARAM_READWRITE));
    params.emplace(PROP_HISTORY_LENGTH, g_param_spec_uint(
        "track-history-length", "Track History Length",
        "Minutes to keep the tracking history.",
        0, 1440, 60, G_PARAM_READWRITE));
    params.emplace(PROP_SMOOTH_BBOX_RATE, g_param_spec_uint(
        "smooth-bbox-rate", "Smooth BoundingBox Rate",
        "Number of last bounding-box updates to average. (requires tracking)",
        1, 1000, 1, G_PARAM_READWRITE));

    params.emplace(PROP_FILTER_NMS, g_param_spec_uint(
        "filter-nms", "Filter NMS",
        "The IOU threshold in percent for the Non-Maximum Suppression (NMS), which filters out overlapping bounding boxes.",
        0, 100, 50, G_PARAM_READWRITE));
    params.emplace(PROP_FILTER_PROB, g_param_spec_uint(
        "filter-prob", "Filter Probability",
        "The probability in percent of detection to consider as valid.",
        0, 100, 50, G_PARAM_READWRITE));
    params.emplace(PROP_FILTER_SHOW, g_param_spec_boolean(
        "filter-show", "Filter Show",
        "Show a yellow box where the filter is applied.",
        false, G_PARAM_READWRITE));
    params.emplace(PROP_FILTER_CLASS, g_param_spec_string(
        "filter-class", "Filter Class",
        "A comma-separated list of classes to filter the detection. Shows all if empty.",
        "", G_PARAM_READWRITE));
    params.emplace(PROP_FILTER_LEFT, g_param_spec_uint(
        "filter-left", "Filter Left",
        "The left edge of the region of interest to filter the detection.",
        0, UINT_MAX, 0, G_PARAM_READWRITE));
    params.emplace(PROP_FILTER_TOP, g_param_spec_uint(
        "filter-top", "Filter Top",
        "The top edge of the region of interest to filter the detection.",
        0, UINT_MAX, 0, G_PARAM_READWRITE));
    params.emplace(PROP_FILTER_WIDTH, g_param_spec_uint(
        "filter-width", "Filter Width",
        "The width of the region of interest to filter the detection.",
        1, UINT_MAX, UINT_MAX, G_PARAM_READWRITE));
    params.emplace(PROP_FILTER_HEIGHT, g_param_spec_uint(
        "filter-height", "Filter Height",
        "The height of the region of interest to filter the detection.",
        1, UINT_MAX, UINT_MAX, G_PARAM_READWRITE));

    params.emplace(PROP_BITMAP_SAVE_DIR, g_param_spec_string(
        "bitmap_save_dir", "Bitmap Save Directory",
        "The directory path to save bitmap images for fewer probability detections.",
        "", G_PARAM_READWRITE));
    params.emplace(PROP_BITMAP_SAVE_MINUTES, g_param_spec_float(
        "bitmap_save_minutes", "Bitmap Save Minutes",
        "Minutes between each bitmap save for fewer probability detections.",
        0, 1000, 5, G_PARAM_READWRITE));
    params.emplace(PROP_BITMAP_SAVE_PROB, g_param_spec_float(
        "bitmap_save_probability", "Bitmap Save Class Probability",
        "The maximum detection probability that triggers the bitmap saving for detections.",
        0, 100, 0, G_PARAM_READWRITE));
    params.emplace(PROP_BITMAP_SAVE_CLASS, g_param_spec_string(
        "bitmap_save_classes", "Bitmap Save Classes",
        "A comma seperated list of classes that triggers the bitmap saving for detections.",
        "", G_PARAM_READWRITE));

    BaseDRPAI::install_properties(params);
}

void DRPAI_Controller::check_save_bmp() {
    // Skip frequent saves
    const auto now = std::chrono::system_clock::now();
    const auto time_duration = std::chrono::duration<float>(now-last_bmp_save).count() / 60.f;
    if (time_duration < bitmap_save_time_between)
        return;

    /* Bitmap saving for fewer probabilities */
    for (auto det : postprocessor->detections.get_last()) {
        if (det.prob < bitmap_save_class_probability) {
            const auto& name = labels.at(det.c);
            if (bitmap_save_classes.empty() || std_find(bitmap_save_classes, name) != bitmap_save_classes.end()) {
                const auto path = bitmap_save_directory + "/image_" + name +
                                  "_" + std::to_string(static_cast<int>(det.prob * 100)) +
                                  "_at_" + std::to_string(static_cast<int>(det.bbox.x)) +
                                  "_" + std::to_string(static_cast<int>(det.bbox.y)) + ".bmp";
                image_mapped_udma->save_bmp(path);
                det.saved_image = true;
                last_bmp_save = now;
                break;
            }
        }
    }
}

/*****************************************
* Function Name     : load_label_file
* Description       : Load label list text file and return the label list that contains the label.
* Arguments         : label_file_name = filename of label list. must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
void DRPAI_Controller::load_label_file(const std::string& label_file_name)
{
    std::ifstream infile(label_file_name);
    if (!infile.is_open())
        throw std::runtime_error("[ERROR] Failed to open label file: " + label_file_name);

    std::string line;
    while (getline(infile,line))
    {
        if (line.empty())
            continue;
        labels.push_back(line);
        if (infile.fail())
            throw std::runtime_error("[ERROR] Failed to read label file: " + label_file_name);
    }
    infile.close();
}

void DRPAI_Controller::send_socket_data() const {
    if (!socket_fd)
        return;

    json_object j;
    j.add("timestamp", elapsed_time::to_string(std::chrono::system_clock::now()));
    j.add("video_rate", video_rate.get_smooth_rate(), 1);
    j.concatenate(drpai->get_json());
    if(det_tracker.active) {
        j.add("detections", det_tracker.get_detections_json(labels));
        j.add("track_history", det_tracker.get_json(labels));
    } else {
        j.concatenate(postprocessor->get_json(labels));
    }
    if (det_filterer.is_active()) {
        j.add("filter", det_filterer.get_json(labels));
    }
    const auto str = j.to_string() + "\n";
    auto r = sendto(socket_fd, str.c_str(), str.size(), 0,
                    reinterpret_cast<const sockaddr *>(&socket_address), sizeof(socket_address));

    if (r < static_cast<ssize_t>(str.size())) {
        std::cerr << "[ERROR] Error sending log to the server: " << std::strerror(errno) << std::endl;
        if (errno == EMSGSIZE) {
            std::cerr << "\tMessage Length: " << str.size() << " - Message sent: " << r << std::endl;
        }
    }
}
