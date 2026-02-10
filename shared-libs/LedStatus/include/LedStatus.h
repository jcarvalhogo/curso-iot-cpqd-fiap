// Created by Josemar Carvalho on 07/02/26.
//

#ifndef GATEWAY_ARDUINO_LEDSTATUS_H
#define GATEWAY_ARDUINO_LEDSTATUS_H

#pragma once
#include <Arduino.h>

/**
 * @file LedStatus.h
 * @brief Non-blocking status LED helper for Arduino/ESP32 projects.
 *
 * This small utility encapsulates the common pattern of using a single LED to
 * communicate system state (boot, connecting, connected, error, etc.) without
 * blocking the main loop with delay().
 *
 * Typical usage:
 * @code
 *   LedStatus led(2);
 *   void setup() {
 *     led.begin();
 *     led.setMode(LedStatus::Mode::BLINK_FAST);
 *   }
 *   void loop() {
 *     led.update(); // call frequently
 *   }
 * @endcode
 *
 * @note This library currently assumes the LED is active-high (HIGH turns it on).
 *       If your board uses an active-low LED, invert at the wiring level or extend
 *       the implementation (see README).
 */

/**
 * @brief Simple non-blocking LED status controller.
 */
class LedStatus {
public:
    /**
     * @brief Predefined LED modes.
     *
     * The blink modes are implemented using millis()-based timing and therefore do
     * not block the main loop.
     */
    enum class Mode {
        OFF,        ///< LED off.
        BLINK_FAST, ///< Fast blink (e.g., booting/connecting).
        BLINK_SLOW, ///< Slow blink (e.g., connected/idle).
        ON          ///< LED on (steady).
    };

    /**
     * @brief Construct a LedStatus bound to a given GPIO pin.
     * @param pin GPIO number used to drive the LED.
     */
    explicit LedStatus(uint8_t pin);

    /**
     * @brief Initialize the GPIO and turn the LED off.
     *
     * Must be called once in setup().
     */
    void begin();

    /**
     * @brief Set the current mode.
     * @param mode Desired LED mode.
     *
     * For OFF/ON the LED state is applied immediately.
     * For blink modes, the internal timer is reset and the LED toggles on update().
     */
    void setMode(Mode mode);

    /**
     * @brief Update the LED state machine (non-blocking).
     *
     * Call this frequently from loop(). It will only toggle the LED when the
     * configured interval elapses.
     */
    void update();

private:
    uint8_t _pin;
    Mode _mode = Mode::OFF;
    uint32_t _lastToggle = 0;
    bool _level = false;

    uint32_t intervalMs() const;
    void write(bool on);
};

#endif // GATEWAY_ARDUINO_LEDSTATUS_H
