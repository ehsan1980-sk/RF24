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
#include <errno.h>     // errno

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
    int fd = ::open(dev_name, O_RDWR);
    if (fd < 0) {
        throw GPIOException("Can't open character device");
        return -1;
    }

    struct gpio_v2_line_request rq;
    rq.offsets[0] = port;
    rq.num_lines = 1;

    struct gpio_v2_line_config config;
    config.flags = GPIO_V2_LINE_FLAG_INPUT;
    rq.config = config;

    int ret = ioctl(fd, GPIO_V2_GET_LINE_IOCTL, &rq);
    if (ret == -1) {
        switch (errno)
        {
            case EBADFD:
                throw GPIOException("[GPIO::read] Can't get line handle from IOCTL (invalid file descriptor)");
                return ret;
            case EFAULT:
                throw GPIOException("[GPIO::read] Can't get line handle from IOCTL (inaccessible memory for request result)");
                return ret;
            case EINVAL:
                throw GPIOException("[GPIO::read] Can't get line handle from IOCTL (invalid request or result type)");
                return ret;
            case ENOTTY:
                throw GPIOException("[GPIO::read] Can't get line handle from IOCTL (file descriptor or request not associated with character device)");
                return ret;
            default:
                throw GPIOException("[GPIO::read] Can't get line handle from IOCTL");
                return ret;
        }
    }

    struct gpio_v2_line_values data;
    ret = ioctl(rq.fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &data);
    if (ret == -1 || rq.fd <= 0) {
        throw GPIOException("Can't get line value from IOCTL");
        return ret;
    }
    ::close(rq.fd);
    ::close(fd);
    return data.bits & 1;
}

void GPIO::write(int port, int value)
{
    int fd = ::open(dev_name, O_RDWR);
    if (fd < 0) {
        throw GPIOException("Can't open character device");
        return;
    }

    struct gpio_v2_line_request rq;
    rq.offsets[0] = port;
    rq.num_lines = 1;

    struct gpio_v2_line_config config;
    config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
    rq.config = config;

    int ret = ioctl(fd, GPIO_V2_GET_LINE_IOCTL, &rq);
    if (ret == -1) {
        switch (errno)
        {
            case EBADFD:
                throw GPIOException("[GPIO::write] Can't get line handle from IOCTL (invalid file descriptor)");
                return;
            case EFAULT:
                throw GPIOException("[GPIO::write] Can't get line handle from IOCTL (inaccessible memory for request result)");
                return;
            case EINVAL:
                throw GPIOException("[GPIO::write] Can't get line handle from IOCTL (invalid request or result type)");
                return;
            case ENOTTY:
                throw GPIOException("[GPIO::write] Can't get line handle from IOCTL (file descriptor or request not associated with character device)");
                return;
            default:
                throw GPIOException("[GPIO::write] Can't get line handle from IOCTL");
                return;
        }
    }

    struct gpio_v2_line_values data;
    data.bits |= value;
    ret = ioctl(rq.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &data);
    if (ret == -1 || rq.fd <= 0) {
        throw GPIOException("Can't set line value from IOCTL");
        return;
    }
    ::close(rq.fd);
    ::close(fd);
}
