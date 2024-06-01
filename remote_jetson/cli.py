#! /usr/bin/env python3

from dataclasses import asdict, dataclass
import time

import requests


MCU_URL = "http://192.168.13.133"


LINE_UP = "\033[1A"
LINE_CLEAR = "\x1b[2K"


class bcolors:
    HEADER = "\033[95m"
    OKBLUE = "\033[94m"
    OKCYAN = "\033[96m"
    OKGREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"


def colored(color: str, text: str) -> str:
    return f"{color}{text}{bcolors.ENDC}"


def colored_bool(b: bool) -> str:
    return colored(bcolors.OKGREEN if b else bcolors.FAIL, str(b))


def clear_line():
    print(LINE_UP, end=LINE_CLEAR)


@dataclass
class McuState:
    power: bool
    recovery: bool


def make_mcu_request(endpoint: str) -> McuState:
    resp = requests.get(f"{MCU_URL}{endpoint}")

    if resp.status_code != requests.codes.ok:
        raise RuntimeError(f"Request {endpoint} failed: {resp.status_code}")

    return McuState(**resp.json())


def toggle_power(state: McuState) -> McuState:
    ep = "/power/on" if not state.power else "/power/off"
    return make_mcu_request(ep)


def toggle_recovery(state: McuState) -> McuState:
    ep = "/recovery/on" if not state.recovery else "/recovery/off"
    return make_mcu_request(ep)


def press_pwr(_=None) -> McuState:
    return make_mcu_request("/press_power_btn")


def get_state(_=None) -> McuState:
    return make_mcu_request("/state")


ACTIONS = {
    "TOGGLE_POWER": toggle_power,
    "TOGGLE_RECOVERY": toggle_recovery,
    "PRESS_PWR_BTN": press_pwr,
    "REFRESH_STATE": get_state,
}


def prompt(max_n: int) -> int:
    while True:
        inp = input("Enter a number: ")
        try:
            v = int(inp.strip())
        except ValueError:
            v = None

        if v is None or v > max_n:
            print("Invalid input, try again...")
            time.sleep(0.5)
            clear_line()
            clear_line()
            continue

        # actions displayed as 1 indexed, reduce
        return v - 1


def main_loop():
    state = get_state()
    action_names = list(ACTIONS.keys())
    action_funcs = list(ACTIONS.values())
    while True:
        s = " ".join(f"{k}: {colored_bool(v)}" for k, v in asdict(state).items())

        nlines = 1
        print(s)

        for i, k in enumerate(action_names):
            nlines += 1
            print(f"({i+1})", k)

        i = prompt(max_n=len(action_funcs))
        print("Executing", colored(bcolors.OKBLUE, action_names[i]))
        state = action_funcs[i](state)
        time.sleep(0.4)
        nlines += 2

        for _ in range(nlines):
            clear_line()


def main():
    try:
        main_loop()
    except KeyboardInterrupt:
        print("\nExiting")


if __name__ == "__main__":
    main()
