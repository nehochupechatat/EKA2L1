/*
 * Copyright (c) 2022 EKA2L1 Team.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <chrono>
#include <cmath>

#include <drivers/input/backend/emu_controller_sdl2.h>
#include <drivers/input/common.h>
#include <common/log.h>

#include <SDL.h>

namespace eka2l1::drivers {
    static controller_button_code SDL_TO_FRONTEND_BUTTON_MAP[] = { 
        CONTROLLER_BUTTON_CODE_A,
        CONTROLLER_BUTTON_CODE_B,
        CONTROLLER_BUTTON_CODE_X,
        CONTROLLER_BUTTON_CODE_Y,
        CONTROLLER_BUTTON_CODE_BACK,
        CONTROLLER_BUTTON_CODE_GUIDE,
        CONTROLLER_BUTTON_CODE_START,
        CONTROLLER_BUTTON_CODE_LB,
        CONTROLLER_BUTTON_CODE_RB,
        CONTROLLER_BUTTON_CODE_LT,
        CONTROLLER_BUTTON_CODE_RT,
        CONTROLLER_BUTTON_CODE_DPAD_UP,
        CONTROLLER_BUTTON_CODE_DPAD_DOWN,
        CONTROLLER_BUTTON_CODE_DPAD_LEFT,
        CONTROLLER_BUTTON_CODE_DPAD_RIGHT,
        CONTROLLER_BUTTON_CODE_MISC1,
        CONTROLLER_BUTTON_CODE_PADDLE1,
        CONTROLLER_BUTTON_CODE_PADDLE2,
        CONTROLLER_BUTTON_CODE_PADDLE3,
        CONTROLLER_BUTTON_CODE_PADDLE4,
        CONTROLLER_BUTTON_CODE_TOUCHPAD,
    };

    emu_controller_sdl2::emu_controller_sdl2()
        : ANALOG_MIN_DIFFERENCE(0.05f)
        , ANALOG_ACKNOWLEDGE_THRESHOLD(0.5f) {
    }

    emu_controller_sdl2::~emu_controller_sdl2() {
        for (auto &gamepad: gamepads) {
            if (gamepad.second.controller) {
                SDL_GameControllerClose(gamepad.second.controller);
            }
        }
    }

    void emu_controller_sdl2::poll() {
        SDL_GameControllerUpdate();

        for (int jid = 0; jid < SDL_NumJoysticks(); jid++) {
            if (SDL_IsGameController(jid)) {
                SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(jid);
                const bool pad_not_exist = (gamepads.count(jid) == 0);

                if (pad_not_exist || (memcmp(gamepads[jid].guid.data, guid.data, sizeof(SDL_JoystickGUID)) != 0)) {
                    if (pad_not_exist) {
                        SDL_GameControllerClose(gamepads[jid].controller);
                    }
                    gamepads[jid].controller = SDL_GameControllerOpen(jid);
                    gamepads[jid].button = std::vector<bool>(SDL_CONTROLLER_BUTTON_MAX, false);
                    gamepads[jid].axis = std::vector<float>(SDL_CONTROLLER_AXIS_MAX, 0.0f);
                    gamepads[jid].guid = guid;
                }

                SDL_GameController *controller = gamepads[jid].controller;

                gamepad_state &current_pad = gamepads[jid];
                for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
                    const std::uint8_t state = SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(static_cast<int>(SDL_CONTROLLER_BUTTON_A) + i));

                    if ((state != 0) != current_pad.button[i]) {
                        current_pad.button[i] = (state != 0) ? true : false;
                        on_button_event(jid, SDL_TO_FRONTEND_BUTTON_MAP[i], state);
                    }
                }

                for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++) {
                    float axis = SDL_GameControllerGetAxis(controller, static_cast<SDL_GameControllerAxis>(static_cast<int>(SDL_CONTROLLER_AXIS_LEFTX) + i)) / 32767.0f;

                    if (std::fabs(axis - current_pad.axis[i]) > ANALOG_MIN_DIFFERENCE) {
                        if (i < SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
                            if (current_pad.axis[i] < ANALOG_ACKNOWLEDGE_THRESHOLD && axis >= ANALOG_ACKNOWLEDGE_THRESHOLD) {
                                on_button_event(jid, CONTROLLER_BUTTON_CODE_JOYSTICK_BASE + i * 2, true);
                            } else if (current_pad.axis[i] >= ANALOG_ACKNOWLEDGE_THRESHOLD && axis < ANALOG_ACKNOWLEDGE_THRESHOLD) {
                                on_button_event(jid, CONTROLLER_BUTTON_CODE_JOYSTICK_BASE + i * 2, false);
                            } else if (current_pad.axis[i] > -ANALOG_ACKNOWLEDGE_THRESHOLD && axis <= -ANALOG_ACKNOWLEDGE_THRESHOLD) {
                                on_button_event(jid, CONTROLLER_BUTTON_CODE_JOYSTICK_BASE + i * 2 + 1, true);
                            } else if (current_pad.axis[i] <= -ANALOG_ACKNOWLEDGE_THRESHOLD && axis > -ANALOG_ACKNOWLEDGE_THRESHOLD) {
                                on_button_event(jid, CONTROLLER_BUTTON_CODE_JOYSTICK_BASE + i * 2 + 1, false);
                            }
                        } else {
                            on_button_event(jid, CONTROLLER_BUTTON_CODE_LEFT_TRIGGER + (i - SDL_CONTROLLER_AXIS_TRIGGERLEFT), (axis > current_pad.axis[i]));
                        }
                        current_pad.axis[i] = axis;
                    }
                }
            } else if (gamepads.count(jid) > 0) {
                // Disconnect
                if (gamepads[jid].controller) {
                    SDL_GameControllerClose(gamepads[jid].controller);
                }
    
                gamepads.erase(jid);
            }
        }
    }

    void emu_controller_sdl2::run() {
        // Who enable this as default? cmonBruh
        SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
        SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "0");
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    
        if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
            return;
        }

        SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

        while (!shall_stop) {
            poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void emu_controller_sdl2::start_polling() {
        shall_stop = false;
        polling_thread = std::make_unique<std::thread>(&emu_controller_sdl2::run, this);
    }

    void emu_controller_sdl2::stop_polling() {
        shall_stop = true;
        polling_thread->join();
    }
}
