/*
 *
 * Copyright (c) 2024, Copyright (c) 2024 TMRh20 & 2bndy5
 * All rights reserved.
 *
 *
 */
#include <linux/gpio.h>
#include <unistd.h>    // close()
#include <fcntl.h>     // open()
#include <sys/ioctl.h> // ioctl()
#include <errno.h>     // errno, strerror()
#include <string.h>    // std::string, strcpy()
#include "gpio.h"

// instantiate some global structs to use as cache
// doing this globally ensures the data struct is zero-ed out
struct gpio_v2_line_request request;
struct gpio_v2_line_values data;
struct gpio_v2_line_config_attribute attr_input;
struct gpio_v2_line_config_attribute attr_output;
struct gpio_v2_line_config_attribute attr_debounce;

// use this struct to keep a track of open file descriptors
struct GlobalCache
{
    const char* chip = "/dev/gpiochip4";
    int fd = -1;

    int getPortOffset(rf24_gpio_pin_t port)
    {
        int offset = -1;
        for (__u32 i = 0; i < request.num_lines; ++i) {
            if (request.offsets[i] == (__u32)port) {
                offset = i;
                break;
            }
        }
        return offset;
    }

    // should be called automatically on program start.
    // Here, we do some one-off configuration.
    GlobalCache()
    {
        if (fd < 0) {
            fd = ::open(chip, O_RDONLY);
            if (fd < 0) {
                chip = "/dev/gpiochip0";
                fd = ::open(chip, O_RDONLY);
                if (fd < 0) {
                    std::string msg = "Can't open device ";
                    msg += chip;
                    throw GPIOException(msg);
                }
            }
        }

        attr_input.attr.id = GPIO_V2_LINE_ATTR_ID_FLAGS;
        attr_input.attr.flags = GPIO_V2_LINE_FLAG_INPUT;

        attr_output.attr.id = GPIO_V2_LINE_ATTR_ID_FLAGS;
        attr_output.attr.flags = GPIO_V2_LINE_FLAG_OUTPUT;

        attr_debounce.attr.id = GPIO_V2_LINE_ATTR_ID_DEBOUNCE;
        attr_debounce.attr.debounce_period_us = 5;

        strcpy(request.consumer, "RF24 lib");
        request.config.num_attrs = 3;
        request.config.attrs[0] = attr_input;
        request.config.attrs[1] = attr_output;
        request.config.attrs[2] = attr_debounce;
    }

    // Should be called automatically on program exit.
    // What we need here is to make sure that the File Descriptors used to
    // control GPIO pins are properly closed.
    ~GlobalCache()
    {
        if (fd >= 0) {
            close(fd);
        }
        if (request.fd > 0) {
            close(request.fd);
        }
    }
} gpio_cache;

GPIO::GPIO()
{
}

GPIO::~GPIO()
{
}

void GPIO::open(rf24_gpio_pin_t port, int DDR)
{
    // check if pin is already in use
    int offset = gpio_cache.getPortOffset(port);
    if (offset < 0) { // pin not in use; add it to cached request
        offset = request.num_lines;
        request.offsets[offset] = port;
        request.num_lines += 1;
    }

    // (re)assign attribute(s) specific to the pin
    if (DDR) {
        attr_output.mask |= (1LL << offset);
        attr_debounce.mask = attr_output.mask;
    }
    else {
        attr_input.mask |= (1LL << offset);
    }

    int ret = ioctl(gpio_cache.fd, GPIO_V2_GET_LINE_IOCTL, &request);
    if (ret == -1) {
        std::string msg = "[GPIO::open] Can't get line handle from IOCTL; ";
        msg += strerror(errno);
        throw GPIOException(msg);
        return;
    }
    ret = ioctl(request.fd, GPIO_V2_LINE_SET_CONFIG_IOCTL, &request.config);
    if (ret == -1) {
        std::string msg = "[gpio::open] Can't set line config; ";
        msg += strerror(errno);
        throw GPIOException(msg);
        return;
    }
}

void GPIO::close(rf24_gpio_pin_t port)
{
    // This is not really used in RF24 convention (designed for embedded apps).
    // Instead rely on gpio_cache destructor (see above)
}

int GPIO::read(rf24_gpio_pin_t port)
{
    if (gpio_cache.fd < 0) {
        throw GPIOException("GPIO device not initialized! Use GPIO::open() first.");
        return -1;
    }

    int offset = gpio_cache.getPortOffset(port);
    if (offset == -1 || request.fd <= 0) {
        throw GPIOException("GPIO pin not initialized! Use GPIO::open() first");
        return -1;
    }

    data.mask = (1LL << offset);

    int ret = ioctl(request.fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &data);
    if (ret == -1) {
        std::string msg = "[GPIO::read] Can't get line value from IOCTL; ";
        msg += strerror(errno);
        throw GPIOException(msg);
        return ret;
    }
    return data.bits & (1LL << offset);
}

void GPIO::write(rf24_gpio_pin_t port, int value)
{
    if (gpio_cache.fd < 0) {
        throw GPIOException("GPIO device not initialized! Use GPIO::open() first.");
        return;
    }

    int offset = gpio_cache.getPortOffset(port);
    if (offset == -1 || request.fd <= 0) {
        throw GPIOException("GPIO pin not initialized! Use GPIO::open() first");
        return;
    }

    data.bits = value ? (1LL << offset) : 0; // set pin output value
    data.mask = (1LL << offset);             // only change value for specified pin

    int ret = ioctl(request.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &data);
    if (ret == -1) {
        std::string msg = "[GPIO::write] Can't set line value from IOCTL; ";
        msg += strerror(errno);
        throw GPIOException(msg);
        return;
    }
}
