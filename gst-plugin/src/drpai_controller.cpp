//
// Created by matin on 21/02/23.
//

#include <memory>
#include <iostream>
#include "drpai_controller.h"


void DRPAI_Controller::open_resources() {
    if (drpai.rate.max_rate == 0) {
        std::cout << "[WARNING] DRPAI is disabled by the zero max framerate." << std::endl;
        return;
    }
    if (drpai.prefix.empty())
        throw std::invalid_argument("[ERROR] The model parameter needs to be set.");

    std::cout << "RZ/V2L DRP-AI Plugin" << std::endl;

    if (multithread)
        process_thread = new std::thread(&DRPAI_Controller::thread_function_loop, this);
    else
        thread_state = Ready;

    /* Obtain udmabuf memory area starting address */
    char addr[1024];
    errno = 0;
    auto fd = open("/sys/class/u-dma-buf/udmabuf0/phys_addr", O_RDONLY);
    if (0 > fd)
        throw std::runtime_error("[ERROR] Failed to open udmabuf0/phys_addr : errno="  + std::string(std::strerror(errno)));
    if ( read(fd, addr, 1024) < 0 )
    {
        close(fd);
        throw std::runtime_error("[ERROR] Failed to read udmabuf0/phys_addr :  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
    }
    uint64_t udmabuf_address = std::strtoul(addr, nullptr, 16);
    close(fd);
    /* Filter the bit higher than 32 bit */
    udmabuf_address &=0xFFFFFFFF;

    image_mapped_udma.map_udmabuf();

    /**********************************************************************/
    /* Inference preparation                                              */
    /**********************************************************************/

    /* Read DRP-AI Object files address and size */
    drpai.open_resource(udmabuf_address);


    std::cout <<"DRP-AI Ready!" << std::endl;
}

int DRPAI_Controller::process_image(uint8_t* img_data) {
    if (drpai.rate.max_rate != 0 && thread_state != Processing) {
        switch (thread_state) {
            case Failed:
            case Unknown:
            case Closing:
                return -1;

            case Ready:
                image_mapped_udma.copy_convert_bgr_to_yuyv(img_data);
                //std::this_thread::sleep_for(std::chrono::milliseconds(50));
                thread_state = Processing;
                if (multithread)
                    v.notify_one();

            default:
                break;
        }
    }

    if(drpai.rate.max_rate != 0 && !multithread)
        try {
            thread_state = Ready;
            thread_function_single();
        }
        catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            thread_state = Failed;
            return -1;
        }

    Image img (DRPAI_IN_WIDTH, DRPAI_IN_HEIGHT, DRPAI_IN_CHANNEL_BGR, img_data);
    video_rate.inform_frame();

    /* Compute the result, draw the result on img and display it on console */
    drpai.corner_text.clear();
    if(show_fps) {
        drpai.corner_text.push_back("Video Rate: " + std::to_string(int(video_rate.get_smooth_rate())) + " fps");
        drpai.add_corner_text();
    }
    drpai.render_detections_on_image(img);
    drpai.render_text_on_image(img);

    return 0;
}

void DRPAI_Controller::release_resources() {
    if(process_thread) {
        {
            std::unique_lock<std::mutex> state_lock(state_mutex);
            thread_state = Closing;
            v.notify_one();
        }
        process_thread->join();
        delete process_thread;
    }

    drpai.release_resource();
}

void DRPAI_Controller::thread_function_loop() {
    try {
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
    {
        std::unique_lock<std::mutex> lock(state_mutex);
        if (thread_state == Closing)
            throw std::exception();

        thread_state = Ready;
        if (multithread) {
            v.wait(lock, [&] { return thread_state != Ready; });
        }
    }

    drpai.run_inference();
}
