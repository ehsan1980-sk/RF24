/*
 *
 * Copyright (c) 2024, Copyright (c) 2024 TMRh20 & 2bndy5
 * All rights reserved.
 *
 *
 */
#include <linux/gpio.h>
#include "gpio.h"
#include <unistd.h>    // close()
#include <fcntl.h>     // open()
#include <sys/ioctl.h> // ioctl()

const char* dev_name = "/dev/gpiochip4";

GPIO::GPIO()
{
}

GPIO::~GPIO()
{
}

void GPIO::open(int port, int DDR)
{
    int fd;
    fd = ::open(dev_name, O_RDONLY);
    if (fd >= 0) {
        ::close(fd);
    }
    else {
        dev_name = "/dev/gpiochip0";
        fd = ::open(dev_name, O_RDONLY);
        if (fd >= 0) {
            ::close(fd);
        }
        else {
            throw GPIOException("can't open /dev/gpiochip");
        }
    }
}

void GPIO::close(int port)
{
}

int GPIO::read(int port)
{
    struct gpio_v2_line_request rq;
    rq.offsets[0] = port;
    rq.num_lines = 1;

    struct gpio_v2_line_config config;
    config.flags = GPIO_V2_LINE_FLAG_INPUT;
    rq.config = config;

    struct gpio_v2_line_values data;

    int fd, ret;
    fd = ::open(dev_name, O_RDONLY);
    if (fd >= 0) {
        ret = ioctl(fd, GPIO_V2_GET_LINE_IOCTL, &rq);
        if (ret == -1 || rq.fd <= 0) {
            throw GPIOException("Can't get line handle from IOCTL");
            return ret;
        }
        ::close(fd);
        ret = ioctl(rq.fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &data);
        if (ret == -1) {
            throw GPIOException("Can't get line value from IOCTL");
            return ret;
        }
        ::close(rq.fd);
        return data.bits & 1;
    }
    return -1;
}

void GPIO::write(int port, int value)
{
    struct gpio_v2_line_request rq;
    rq.offsets[0] = port;
    rq.num_lines = 1;

    struct gpio_v2_line_config config;
    config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
    rq.config = config;

    int fd, ret;
    fd = ::open(dev_name, O_RDONLY);
    if (fd < 0) {
        throw GPIOException("Can't open dev");
        return;
    }
    ret = ioctl(fd, GPIO_V2_GET_LINE_IOCTL, &rq);
    if (ret == -1 || rq.fd <= 0) {
        throw GPIOException("Can't get line handle from IOCTL");
        return;
    }
    ::close(fd);

    struct gpio_v2_line_values data;
    data.bits |= value;
    ret = ioctl(rq.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &data);
    if (ret == -1) {
        throw GPIOException("Can't set line value from IOCTL");
        return;
    }
    ::close(rq.fd);
}
